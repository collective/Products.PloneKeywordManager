from plone import api
from plone.app.testing import setRoles
from plone.app.testing import TEST_USER_ID
from Products.PloneKeywordManager.browser.interfaces import IPloneKeywordManagerLayer
from Products.PloneKeywordManager.testing import PLONEKEYWORDMANAGER_INTEGRATION_TESTING
from zope import interface
from zope.component import getMultiAdapter

import unittest


class BaseIntegrationTestCase(unittest.TestCase):

    layer = PLONEKEYWORDMANAGER_INTEGRATION_TESTING

    def setUp(self):
        self.portal = self.layer["portal"]
        self.request = self.layer["request"]
        self.markRequestWithLayer()
        setRoles(self.portal, TEST_USER_ID, ["Manager"])
        self.pkm = api.portal.get_tool("portal_keyword_manager")

    def markRequestWithLayer(self):
        # to be removed when p.a.testing will fix https://dev.plone.org/ticket/11673
        # Well, that's a 404, but p.a.testing 4.2.2 -> 4.2.4 has a release note that it was fixed
        #  Plone 4.3.3 release notes.
        request = self.layer["request"]
        interface.alsoProvides(request, IPloneKeywordManagerLayer)


class PKMTestCase(BaseIntegrationTestCase):
    def _action_change(self, keywords, changeto, field="Subject"):
        """calls changeKeywords method from  prefs_keywords_view"""
        view = getMultiAdapter((self.portal, self.request), name="prefs_keywords_view")
        view.changeKeywords(keywords, changeto, field)

    def _action_delete(self, keywords, field="Subject"):
        """calls deleteKeywords method from  prefs_keywords_view"""
        view = getMultiAdapter((self.portal, self.request), name="prefs_keywords_view")
        view.deleteKeywords(keywords, field)
