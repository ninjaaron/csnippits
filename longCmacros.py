#!/usr/bin/env python3
import re
import fileinput

MACRODEF = re.compile(r"#define\s+\w+\((.*?)\)\s*")
ARGSEP = re.compile(r"\s*,\s*")
W = re.compile(r"\w")


class ReIterator:
    def __init__(self, iterable):
        """an iterator where values can be pushed back. Basically just a
        stack you can use in a for loop.
        """
        self.iterator = iter(iterable)
        self.stack = []

    def push(self, value):
        self.stack.append(value)

    def __iter__(self):
        return self

    def __next__(self):
        if self.stack:
            return self.stack.pop()
        return next(self.iterator)


def bodyiter(lines: ReIterator):
    """collect the body of a multi-line macro. The body ends as soon as
    soon as a new preprocessor diretive is encountered (a line starting
    with #)

    A macro may also be closed explicitely with the `#enddefine`
    directive, which will be converted into an empty line in the
    generated code.
    """
    for line in lines:
        if line.startswith("#enddefine"):
            lines.push("\n")
            return
        if line.startswith("#"):
            lines.push(line)
            return
        yield line.rstrip()


def argsub(m: re.Match):
    """The only syntactic difference between long macros and normal
    macros in the body is that tokens are interpolated into other names
    without the normal ## pasting syntax. This could cause problems
    inside of string literals.

    This can be disabled with by setting `autopaste=False` when
    calling `mkmacro`.
    """
    body = m.string
    start = end = ""
    if m.start() != 0 and W.match(body[m.start()-1]):
        start = " ## "
    if m.end() != len(body) and W.match(body[m.end()]):
        end = " ## "
    return ''.join((start, m.group(0), end))


def mkmacro(match: re.Match, bodylines, autopaste=True):
    """Takes the regex match that looked like the beginning of a
    multi-line macro and the lines from the body and adds trailing
    backslashes.

    By default, tokens are pasted into adjoining words. (see `argsub`
    docstring) Dissable with `autopaste=False`.
    """
    body = ' \\\n'.join(bodylines)
    if autopaste:
        args = ARGSEP.split(match.group(1))
        body = re.sub('|'.join(args), argsub, body)
    return "%s \\\n%s" % (match.group(0).rstrip(), body)


def main():
    lines = ReIterator(fileinput.input())
    for line in lines:
        macromatch = MACRODEF.fullmatch(line)
        if macromatch:
            output = mkmacro(macromatch, bodyiter(lines))
            print(output)
        else:
            print(line, end="")


if __name__ == "__main__":
    main()
