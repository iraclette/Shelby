#!/usr/bin/env python3
"""Lists every translatable string in src/ (tr(), QObject::tr(), QT_TR_NOOP()), so the
translation files in resources/translations/ can be checked for missing entries.
Usage: python3 scripts/translations.py [--missing ka|ru|en]"""
import json, pathlib, re, sys

ROOT = pathlib.Path(__file__).parent.parent
CALL = re.compile(r'(?:\bQObject::tr|\btr|QT_TR_NOOP)\(\s*((?:"(?:[^"\\]|\\.)*"\s*)+)')
LITERAL = re.compile(r'"((?:[^"\\]|\\.)*)"')

def unescape(text):
    return re.sub(r'\\(.)', lambda m: {'n': '\n', 't': '\t'}.get(m.group(1), m.group(1)), text)

def strings():
    found = {}
    for path in sorted((ROOT / 'src').glob('*.[ch]*')):
        source = path.read_text()
        for m in CALL.finditer(source):
            text = ''.join(unescape(part) for part in LITERAL.findall(m.group(1)))
            found.setdefault(text, path.name)
    return found

if __name__ == '__main__':
    found = strings()
    if len(sys.argv) == 3 and sys.argv[1] == '--missing':
        have = json.loads((ROOT / 'resources/translations' / f'{sys.argv[2]}.json').read_text())
        need = [s for s in found if s not in have and (sys.argv[2] != 'en' or '%n' in s)]
        print('\n'.join(need) if need else 'nothing missing')
    else:
        for text, where in found.items():
            print(json.dumps(text, ensure_ascii=False), '  #', where)
