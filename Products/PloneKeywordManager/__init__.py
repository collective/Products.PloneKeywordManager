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

# Zope imports
from AccessControl.Permission import registerPermissions

# CMF imports
from Products.CMFCore.utils import ToolInit

# Sibling imports
from Products.PloneKeywordManager import config
from Products.PloneKeywordManager import tool

global cmf_keyword_manager_globals

import pkg_resources

try:
    pkg_resources.get_distribution('plone.dexterity')
except pkg_resources.DistributionNotFound:
    HAS_DEXTERITY = False
else:
    HAS_DEXTERITY = True


cmf_keyword_manager_globals = globals()

from zope.i18nmessageid import MessageFactory
keywordmanagerMessageFactory = MessageFactory('Products.PloneKeywordManager')
import logging
logger = logging.getLogger("Products.PloneKeywordManager")


registerPermissions(
    [(config.MANAGE_KEYWORDS_PERMISSION, [])],
    ('Manager', 'Site Administrator'))


def initialize(context):

    new_tool = ToolInit(
        config.TOOL_NAME, tools=(tool.PloneKeywordManager, ), icon='tool.gif')
    new_tool.initialize(context)
