from plone import api
from plone.app.testing import setRoles
from plone.app.testing import TEST_USER_ID
from Products.PloneKeywordManager.testing import PLONEKEYWORDMANAGER_INTEGRATION_TESTING
from zope.component import getMultiAdapter

import unittest


class BaseIntegrationTestCase(unittest.TestCase):

    layer = PLONEKEYWORDMANAGER_INTEGRATION_TESTING

    def setUp(self):
        self.portal = self.layer["portal"]
        self.request = self.layer["request"]
        setRoles(self.portal, TEST_USER_ID, ["Manager"])
        self.pkm = api.portal.get_tool("portal_keyword_manager")


class PKMTestCase(BaseIntegrationTestCase):
    def _action_change(self, keywords, changeto, field="Subject"):
        """calls changeKeywords method from  prefs_keywords_view"""
        view = getMultiAdapter((self.portal, self.request), name="prefs_keywords_view")
        view.changeKeywords(keywords, changeto, field)

    def _action_delete(self, keywords, field="Subject"):
        """calls deleteKeywords method from  prefs_keywords_view"""
        view = getMultiAdapter((self.portal, self.request), name="prefs_keywords_view")
        view.deleteKeywords(keywords, field)
