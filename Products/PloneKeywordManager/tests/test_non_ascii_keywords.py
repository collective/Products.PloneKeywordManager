# -*- coding: utf-8 -*-

from Products.PloneKeywordManager.tests.base import IntegrationTestCase


class NonAsciiKeywordsTestCase(IntegrationTestCase):

    def setUp(self):
        super(NonAsciiKeywordsTestCase, self).setUp()
        self.portal.invokeFactory('Document', 'keyword_doc')
        self.document = self.portal['keyword_doc']
        self.document.edit(subject=
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

    def test_only_one_index_is_updated(self):
        def search(**kw):
            return [r.getObject() for r in self.portal.portal_catalog(**kw)]
        self.document.edit(title='Foo')
        self.assertEqual(self.document.Title(), 'Foo')
        # setting the attribute directly...
        self.document.title = 'Bar'
        self.assertEqual(self.document.Title(), 'Bar')
        # ...should not cause the field to be reindexed.
        self.assertEqual(search(Title='Foo'), [self.document])
        self.assertEqual(search(Title='Bar'), [])
        # and remapping Keywords should reindex 'Subject'...
        self.assertEqual(search(Subject=u'Fr\\xfchst\\xfcck'), [self.document])
        self.assertEqual(search(Subject=u'Fr\\xfchessen'), [self.document])
        self._action_delete([u'Fr\\xfchst\\xfcck',])
        self._action_change([u'Fr\\xfchessen'], u'Zen')
        self.assertEqual(search(Subject=u'Fr\\xfchst\\xfcck'), [])
        self.assertEqual(search(Subject=u'Zen'), [self.document])
        # ...but not 'Title'...
        self.assertEqual(search(Title='Bar'), [])
        # ...until the content is completely reindexed
        self.document.reindexObject()
        self.assertEqual(search(Title='Bar'), [self.document])

    def test_getscoredmatches(self):
        self.pkm.getScoredMatches(u'foo', ['foo', u'bar', 'baz'], 7, 0.6)
