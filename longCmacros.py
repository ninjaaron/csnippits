#!/usr/bin/env python3
import re
import fileinput

MACRODEF = re.compile(r"#define\s+\w+\((.*?)\)\s*")
ARGSEP = re.compile(r"\s*,\s*")
W = re.compile(r"\w")


class ReIterate:
    def __init__(self, iterable):
        """an iterator where values can be pushed back"""
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


def bodyiter(lines):
    for line in lines:
        if line.startswith("#enddefine"):
            lines.push("\n")
            return
        if line.startswith("#"):
            lines.push(line)
            return
        yield line.rstrip()


def argsub(m: re.Match):
    body = m.string
    start = end = ""
    if m.start() != 0 and W.match(body[m.start()-1]):
        start = " ## "
    if m.end() != len(body) and W.match(body[m.end()]):
        end = " ## "
    return ''.join((start, m.group(0), end))


def mkmacro(match: re.Match, lines: ReIterate, tabstop=8):
    args = ARGSEP.split(match.group(1))
    bodylines = list(bodyiter(lines))
    prebody = ' \\\n'.join(bodylines)
    body = re.sub('|'.join(args), argsub, prebody)
    return "%s \\\n%s" % (match.group(0).rstrip(), body)


def main():
    lines = ReIterate(fileinput.input())
    for line in lines:
        macromatch = MACRODEF.fullmatch(line)
        if macromatch:
            output = mkmacro(macromatch, lines)
            print(output)
        else:
            print(line, end="")


if __name__ == "__main__":
    main()
