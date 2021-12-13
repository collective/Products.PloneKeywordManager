from plone import api
from plone.app.discussion.interfaces import IConversation
from plone.app.discussion.interfaces import IDiscussionSettings
from Products.PloneKeywordManager.tests.base import PKMTestCase
from zope.component import createObject

import unittest

class NonAsciiKeywordsTestCase(PKMTestCase):
    def setUp(self):
        super().setUp()
        self.document = api.content.create(
            container=self.portal, type="Document", id="document"
        )
        self.document.subject = [
            "Fr\\xfchst\\xfcck",
            "Mitagessen",
            "Abendessen",
            "Fr\\xfchessen",
        ]
        self.document.reindexObject()

    # def test_show_non_ascii_bugs(self):
    #     """
    #     this demonstrates two bugs in the UI, which appear only with non-ASCII keywords.
    #     it is commented out, as will obviously fail after the bugfixes.
    #     """
    #     self.assertRaises(
    #         UnicodeDecodeError,
    #         self._action_change,
    #         [u"Fr\\xfchst\\xfcck".decode("utf-8"), "Mittagessen"],
    #         "Abendessen",
    #     )
    #     self.assertRaises(
    #         UnicodeDecodeError,
    #         self._action_delete,
    #         [u"Fr\\xfchst\\xfcck".decode("utf-8")],
    #     )

    def test_pref_keywords_action_change_keywords(self):
        """test the bugfix for prefs_keywords_action_change when keywords
        contains at least one element with non ASCII characters"""
        self._action_change(["Fr\\xfchst\\xfcck", "Mittagessen"], "Abendessen")

    def test_pref_keywords_action_change_changeto(self):
        """test the bugfix for prefs_keywords_action_change when change to
        contains non ASCII characters"""
        self._action_change(["Fr\\xfchst\\xfcck", "Mittagessen"], "Fr\\xfchessen")

    def test_pref_keywords_action_delete(self):
        """test the bugfix for prefs_keywords_action_delete"""
        self._action_delete(["Fr\\xfchst\\xfcck"])

    def test_only_one_index_is_updated(self):
        def search(**kw):
            return [r.getObject() for r in self.portal.portal_catalog(**kw)]

        self.document.title = "Foo"
        self.document.reindexObject()
        self.assertEqual(self.document.Title(), "Foo")
        self.assertEqual(search(Title="Foo"), [self.document])

        # setting the attribute directly...
        self.document.title = "Bar"
        self.assertEqual(self.document.Title(), "Bar")

        # ...should not cause the field to be reindexed.
        self.assertEqual(search(Title="Foo"), [self.document])
        self.assertEqual(search(Title="Bar"), [])

        # and remapping Keywords should reindex 'Subject'...
        self.assertEqual(search(Subject="Fr\\xfchst\\xfcck"), [self.document])
        self.assertEqual(search(Subject="Fr\\xfchessen"), [self.document])
        self._action_delete(["Fr\\xfchst\\xfcck"])
        self._action_change(["Fr\\xfchessen"], "Zen")
        self.assertEqual(search(Subject="Fr\\xfchst\\xfcck"), [])
        self.assertEqual(search(Subject="Zen"), [self.document])

        # ...but not 'Title'...
        self.assertEqual(search(Title="Bar"), [])

        # ...until the content is completely reindexed
        self.document.reindexObject()
        self.assertEqual(search(Title="Bar"), [self.document])

    def test_getscoredmatches(self):
        self.pkm.getScoredMatches("foo", ["foo", "bar", "baz"], 7, 0.6)

    @unittest.skip("flaky test, randomly fails")
    def test_monovalued_keyword(self):
        # use language only because it is the only monovalued field available by default
        self.portal.portal_catalog.addIndex("Language", "KeywordIndex")
        self.document.language = "en"
        self._action_change("en", "en-US", field="Language")
        self.assertEqual(self.document.language, "en-US")

    def test_discussion_indexes_updated(self):
        # Allow discussion
        api.portal.set_registry_record(
            name="globally_enabled",
            value=True,
            interface=IDiscussionSettings,
        )
        self.document.allow_discussion = True
        conversation = IConversation(self.document)
        comment = createObject("plone.Comment")
        comment.text = "Comment text"
        conversation.addComment(comment)
        self._action_delete(["Fr\\xfchst\\xfcck"])
        self.assertNotIn("Fr\\xfchst\\xfcck", self.document.subject)
        self.assertNotIn("Fr\\xfchst\\xfcck", self.pkm.getKeywords())
