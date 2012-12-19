# convenience Makefile to run tests and QA tools
# options: zc.buildout options
# src: source path
# minimum_coverage: minimun test coverage allowed
# pep8_ignores: ignore listed PEP 8 errors and warnings
# max_complexity: maximum McCabe complexity allowed

SHELL = /bin/sh

options = -N -q -t 3
src = Products/PloneKeywordManager/
minimum_coverage = 100
pep8_ignores = E501
max_complexity = 12

python-validation:
	@echo Validating Python files
	bin/flake8 --ignore=$(pep8_ignores) --max-complexity=$(max_complexity) $(src)

quality-assurance: python-validation
	@echo Quality assurance
	./coverage.sh $(minimum_coverage)

install:
	mkdir -p buildout-cache/downloads
	python bootstrap.py -c travis.cfg
	bin/buildout -c travis.cfg $(options)

tests:
	bin/test
