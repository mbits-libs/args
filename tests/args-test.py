#!/usr/bin/env python

from __future__ import print_function
import os, sys, subprocess

if sys.stdout.isatty() and os.name != 'nt':
	COLORS = dict(
			list(zip([
				'grey',
				'red',
				'green',
				'yellow',
				'blue',
				'magenta',
				'cyan',
				'white',
				],
				list(range(30, 38))
				))
			)
	COLORS['grey'] += 60

	RESET = '\033[0m'

	def colored(text, color = None):
		fmt_str = '\033[%dm%s'
		if color is not None:
			text = fmt_str % (COLORS[color], text)

		text += RESET
		return text
else:
	def colored(text, color = None):
		return text

exe = sys.argv[1]

print(colored(exe, 'grey'))

def mktest(result, title):
  return (int(result), title)

def tests():
  p = subprocess.Popen([exe], stdout=subprocess.PIPE)
  out, err = p.communicate()
  if p.returncode:
    sys.exit(p.returncode)

  out = out.decode('utf-8')
  return [mktest(*line.split(':', 1)) for line in out.split('\n') if line]

tests_failed = 0
test_id = 0
tests = tests()
for result, title in tests:
  print(colored("[ RUN      ]", "green"), colored(title, "grey"))
  sys.stdout.flush()
  p = subprocess.Popen([exe, "%s" % test_id])
  test_id += 1
  p.communicate()
  failed = p.returncode != 0
  should_fail = result != 0
  if should_fail != failed:
    tests_failed += 1
    print('''Expected equality of these values:
  expected
    Which is: {result}
  actual
    Which is: {returncode}'''.format(result = result, returncode=p.returncode))
    print(colored("[  FAILED  ]", "red"), colored(title, "grey"))
  else:
    print(colored("[       OK ]", "green"), colored(title, "grey"))

print("Result: {failed}/{total} failed".format(failed=tests_failed, total=len(tests)))
sys.exit(tests_failed)