# -*- coding: utf-8 -*-
import unittest2 as unittest

from plone.app.testing import TEST_USER_ID
from plone.app.testing import setRoles

from Products.CMFCore.utils import getToolByName
from Products.PloneKeywordManager.testing import INTEGRATION_TESTING

class IntegrationTestCase(unittest.TestCase):

    layer = INTEGRATION_TESTING

    def setUp(self):
        self.portal = self.layer['portal']
        setRoles(self.portal, TEST_USER_ID, ['Manager'])

        self.pkm = getToolByName(self.portal, 'portal_keyword_manager')

    def _action_change(self, keywords, changeto, field='Subject'):
        """ calls prefs_keywords_action_change.py """
        self.portal.prefs_keywords_action_change(keywords, changeto, field)

    def _action_delete(self, keywords, field='Subject'):
        """ calls prefs_keywords_action_delete.cpy """
        self.portal.prefs_keywords_action_delete(keywords, field)
