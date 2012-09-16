# -*- coding: utf-8 -*-

import unittest2 as unittest

from plone.app.testing import TEST_USER_ID
from plone.app.testing import setRoles

from Products.PloneKeywordManager.testing import INTEGRATION_TESTING

from Products.CMFCore.utils import getToolByName


class NonAsciiKeywordsTestCase(unittest.TestCase):

    layer = INTEGRATION_TESTING

    def setUp(self):
        self.portal = self.layer['portal']
        setRoles(self.portal, TEST_USER_ID, ['Manager'])

        self.pkm = getToolByName(self.portal, 'portal_keyword_manager')
        self.portal.setSubject(
            [u'Fr\\xfchst\\xfcck',
              'Mitagessen',
              'Abendessen',
             u'Fr\\xfchessen',
            ])

    def _action_change(self, keywords, changeto, field='Subject'):
        """ calls prefs_keywords_action_change.py """
        self.portal.prefs_keywords_action_change(keywords, changeto, field)

    def _action_delete(self, keywords, field='Subject'):
        """ calls prefs_keywords_action_delete.cpy """
        self.portal.prefs_keywords_action_delete(keywords, field)

    # def test_show_non_ascii_bugs(self):
    #     """
    #     this demonstrates two bugs in the UI, which appear only with non-ASCII keywords.
    #     it is commented out, as will obviously fail after the bugfixes.
    #     """
    #     self.assertRaises(UnicodeDecodeError, self._action_change,
    #         [u'Fr\\xfchst\\xfcck'.decode('utf-8'), 'Mittagessen', ], 'Abendessen', )
    #     self.assertRaises(UnicodeDecodeError, self._action_delete, [u'Fr\\xfchst\\xfcck'.decode('utf-8'), ])

    def test_pref_keywords_action_change_keywords(self):
        """ test the bugfix for prefs_keywords_action_change when keywords
        contains at least one element with non ASCII characters """
        self._action_change([u'Fr\\xfchst\\xfcck', 'Mittagessen', ], 'Abendessen')

    def test_pref_keywords_action_change_changeto(self):
        """ test the bugfix for prefs_keywords_action_change when changeto contains non ASCII characters """
        self._action_change([u'Fr\\xfchst\\xfcck', 'Mittagessen', ], u'Fr\\xfchessen')

    def test_pref_keywords_action_delete(self):
        """ test the bugfix for prefs_keywords_action_delete """
        self._action_delete([u'Fr\\xfchst\\xfcck', ])

    def test_getscoredmatches(self):
        self.pkm.getScoredMatches(u'foo', ['foo', u'bar', 'baz'], 7, 0.6)
