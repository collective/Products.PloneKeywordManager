#!/usr/bin/python
# gendoc.py
# Generate HTML documentation
# Accepts one option: --selfcontained
# (can be abbreviated as long as is unique, so `s' is enough).
import re, os, sys
from cgi import escape as q

html_head = """\
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN"
    "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
<head><title>Levenshtein %s</title></head>
<body>
"""

selfcontained = len(sys.argv) > 1 \
                and 'selfcontained'.startswith(sys.argv[1].strip('-'))

def format_synopsis(synopsis, link=False):
    lst = synopsis.split('\n')
    for i, s in zip(range(len(lst)), lst):
        m = re.match(r'(?P<func>\w+)(?P<args>.*)', s)
        args = re.sub(r'([a-zA-Z]\w+)', r'<var>\1</var>', m.group('args'))
        func = m.group('func')
        if link:
            func = '<a href="#%s">%s</a>' % (func, func)
        lst[i] = func + args
    return '<br/>\n'.join(lst)

fh = os.popen('pydoc Levenshtein')
doc = fh.read()
fh.close()

intro = re.search('^DESCRIPTION(.*)^FUNCTIONS', doc, re.M | re.S)
intro = re.sub('\n    ', '\n', intro.group(1).strip())
doc = re.search('^FUNCTIONS(.*)^DATA', doc, re.M | re.S)
doc = re.sub('\n    ', '\n', doc.group(1).strip())

desc = ''
for p in intro.split('\n\n'):
    if not re.search('^- ', p, re.M):
        desc += '<p>%s</p>\n' % q(p)
        continue
    p = re.split('(?m)^- ', p)
    if p[0]:
        desc += '<p>%s</p>\n' % q(p[0].strip())
    del p[0]
    desc += '<ul>%s</ul>\n' \
            % '\n'.join(['<li>%s</li>' % q(x.strip()) for x in p])

ltoc = '<ul class="ltoc">\n'
text = '<dl>\n'
for func in doc.split('\n\n'):
  func = func.strip()
  name, func = func.split('\n', 1)
  name = re.sub(r'\(.*', '', name)
  func = re.sub('\n    ', '\n', func)
  intro, synopsis, main = func.split('\n\n', 2)
  func = intro.strip() + '\n\n' + main
  text += '<dt id="%s">%s</dt>\n' % (name, format_synopsis(synopsis))
  ltoc += '<li>%s</li>\n' % format_synopsis(synopsis, True)
  text += '<dd>'
  for p in func.split('\n\n'):
      if not re.search('^>>>', p, re.M):
          text += '<p>%s</p>\n' % q(p)
          continue
      x, p = re.split('(?m)^>>>', p, 1)
      if x:
          text += '<p>%s</p>\n' % q(x.strip())
      text += '<pre>\n%s\n</pre>\n' % q('>>>' + p)
  text += '</dd>\n'
text += '</dl>\n'
ltoc += '</ul>\n'

fh = file('Levenshtein.html', 'w')
if selfcontained:
    fh.write(html_head % 'API')
    fh.write('<h1>Levenshtein API</h1>\n')
fh.write(desc)
fh.write('<p><b>Functions:</b></p>\n')
fh.write(ltoc)
fh.write('<hr/>\n')
fh.write(text)
if selfcontained:
    fh.write('</body>\n</html>\n')
fh.close()

for f in ['NEWS']:
    fh = file(f, 'r')
    doc = fh.read()
    fh.close()

    fh = file(f + '.xhtml', 'w')
    if selfcontained:
        fh.write(html_head % f)
    fh.write('<h1>Levenshtein %s</h1>\n\n' % f)
    fh.write('<pre class="main">\n')
    fh.write(doc)
    fh.write('</pre>\n')
    if selfcontained:
        fh.write('</body>\n</html>\n')
    fh.close()
