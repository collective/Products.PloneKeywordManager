# -*- coding: utf-8 -*-

from Products.PloneKeywordManager.tests.base import IntegrationTestCase

class NonAsciiKeywordsTestCase(IntegrationTestCase):

    def setUp(self):
        super(NonAsciiKeywordsTestCase, self).setUp()
        self.portal.setSubject(
            [u'Fr\\xfchst\\xfcck',
              'Mitagessen',
              'Abendessen',
             u'Fr\\xfchessen',
            ])

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
