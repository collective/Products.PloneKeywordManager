# -*- coding: utf-8 -*-
from plone.app.testing import logout
from Products.PloneKeywordManager.config import PROJECTNAME
from Products.PloneKeywordManager.tests.base import BaseIntegrationTestCase
from zope.component import getMultiAdapter


class ControlPanelTestCase(BaseIntegrationTestCase):

    def setUp(self):
        super(ControlPanelTestCase, self).setUp()
        self.controlpanel = self.portal['portal_controlpanel']

    def test_controlpanel_has_view(self):
        view = getMultiAdapter(
            (self.portal, self.request), name='prefs_keywords_view')
        view = view.__of__(self.portal)
        self.assertTrue(view())

    def test_controlpanel_view_is_protected(self):
        from AccessControl import Unauthorized
        logout()
        with self.assertRaises(Unauthorized):
            self.portal.restrictedTraverse('@@prefs_keywords_view')

    def test_controlpanel_installed(self):
        actions = [a.getAction(self)['id']
                   for a in self.controlpanel.listActions()]
        self.assertIn('keywordmanager', actions)

    def test_controlpanel_removed_on_uninstall(self):
        qi = self.portal['portal_quickinstaller']
        qi.uninstallProducts(products=[PROJECTNAME])
        actions = [a.getAction(self)['id']
                   for a in self.controlpanel.listActions()]
        self.assertNotIn('keywordmanager', actions)
