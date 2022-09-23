###
##
# Plone-Keyword-Manager
#
# Copyright (C) 2003 Maik Jablonski (maik.jablonski@uni-bielefeld.de)
# Copyright (C) 2005 gocept gmbh & co. kg (ct@gocept.com)
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
#
# Uses Levenshtein-C-Wrapper for Python
# because it will speed things up ~ 10 times than python-difflib.
#
# More info about Levenshtein on:
# http://trific.ath.cx/resources/python/levenshtein/
##
###
from zope.i18nmessageid import MessageFactory

import logging


keywordmanagerMessageFactory = MessageFactory("Products.PloneKeywordManager")
logger = logging.getLogger("Products.PloneKeywordManager")
