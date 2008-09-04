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
# Uses Levenshtein-C-Wrapper for Python if installed,
# if not, use python-difflib. Levenshtein is recommended,
# because it will speed things up ~ 10 times.
#
# More info about Levenshtein on:
# http://trific.ath.cx/resources/python/levenshtein/
##
###

# Zope imports
from AccessControl.Permission import registerPermissions

# CMF imports
from Products.CMFCore.DirectoryView import registerDirectory
from Products.CMFCore.utils import ToolInit

# Sibling imports
from Products.PloneKeywordManager import config
from Products.PloneKeywordManager import tool

global cmf_keyword_manager_globals
cmf_keyword_manager_globals=globals()

registerPermissions([(config.MANAGE_KEYWORDS_PERMISSION,[])],
    ('Manager',))

def initialize(context):
    registerDirectory('skins', cmf_keyword_manager_globals)
    try:
        ToolInit('Plone Keyword Manager Tool',
                 tools=(tool.PloneKeywordManager, ),
                 icon='tool.gif').initialize(context)
    except TypeError:
        # BBB product_name is required by CMF 1.4.x (part of Plone 2.0.x).
        ToolInit('Plone Keyword Manager Tool',
                 tools=(tool.PloneKeywordManager, ),
                 product_name='PloneKeywordManager',
                 icon='tool.gif').initialize(context)

