from plone.app.testing import logout
from Products.PloneKeywordManager.tests.base import BaseIntegrationTestCase
from zope.component import getMultiAdapter


class ControlPanelTestCase(BaseIntegrationTestCase):
    def setUp(self):
        super().setUp()
        self.controlpanel = self.portal["portal_controlpanel"]

    def test_controlpanel_has_view(self):
        view = getMultiAdapter((self.portal, self.request), name="prefs_keywords_view")
        self.assertTrue(view())

    def test_controlpanel_view_is_protected(self):
        from AccessControl import Unauthorized

        logout()
        with self.assertRaises(Unauthorized):
            self.portal.restrictedTraverse("@@prefs_keywords_view")
