# -*- coding: utf-8 -*-
import json
import typing

from plone import api
from Products.CMFCore.utils import getToolByName
from Products.Five import BrowserView
from Products.Five.browser.pagetemplatefile import ViewPageTemplateFile
from Products.PloneKeywordManager import keywordmanagerMessageFactory as _
from Products.PloneKeywordManager.compat import to_str
from Products.CMFPlone.utils import safe_encode
from Products.CMFPlone.PloneBatch import Batch

import logging


logger = logging.getLogger("Products.PloneKeywordManager")


PLONE_5 = api.env.plone_version() >= "5"


class PrefsKeywordsView(BrowserView):
    """
    A view to manage the keywords in the portal
    """

    template = ViewPageTemplateFile("prefs_keywords_view.pt")

    def __init__(self, context, request):
        super().__init__(context, request)
        self.pkm = getToolByName(self.context, "portal_keyword_manager")


    def __call__(self):
        self.is_plone_5 = PLONE_5
        if not self.request.form.get(
            "form.button.Merge", ""
        ) and not self.request.form.get("form.button.Delete", ""):
            return self.template({})

        keywords = self.request.get("keywords", None)
        field = self.request.get("field", None)

        if not keywords:
            message = _(u"Please select at least one keyword")
            return self.doReturn(message, "error", field=field)

        if not field or field not in self.pkm.getKeywordIndexes():
            message = _(u"Please select a valid keyword field")
            return self.doReturn(message, "error", field=field)

        if "form.button.Merge" in self.request.form:
            # We should assume there is a 'changeto' filled
            changeto = self.request.get("changeto", None)
            if not changeto:
                message = _(u"Please provide a new term")
                return self.doReturn(message, "error", field=field)

            return self.changeKeywords(keywords, changeto, field)

        if "form.button.Delete" in self.request.form:
            return self.deleteKeywords(keywords, field)

    def getKeywords(self, indexName, b_start=0, b_size=30):
        """
        :param indexName the name of the index we want to get all keywords for.
        :param b_start: Batching support - page to start from
        :param b_size: Batching support - size of page
        :return: a Products.CMFPlone Batch object containing the entire list of keywords.
        """

        return Batch(self.pkm.getKeywords(indexName=indexName),
                     b_size, b_start)

    def getKeywordIndexes(self):
        return self.pkm.getKeywordIndexes()

    def getScoredMatches(self, keyword, batch, num_similar, score):
        return self.pkm.getScoredMatches(keyword, batch, num_similar, score, context=self.context)

    def changeKeywords(self, keywords, changeto, field):
        """
          All keywords listed in the list 'keywords' are deleted from the field 'field' and it's KeywordIndex.
          All objects that contain at least one of the 'keywords' in the field are then given the keyword 'changeto'

          Example:
              There are keywords 'foo', 'Foo', 'foo1' and 'foo_' in the KeywordIndex 'subject'
              we want to unify them to a single keyword 'Foo'
              keywords = ['foo', 'foo1', 'foo_']
              changeto = 'Foo'

              we search for all objects with the keywords 'foo', 'foo1', or 'foo_' in the subject field
              we remove these keywords from the field 'subect'
              we then add the keyword 'Foo' (if it didn't alreay exist) to the subject field.
              we save all those objects.

            we should also rebuild the index, but hey... that's work.
        """
        changed_objects = self.pkm.change(
            keywords, changeto, context=self.context, indexName=field
        )
        msg = _(
            "msg_changed_keywords",
            default=u"Changed ${from} to ${to} for ${num} object(s).",
            mapping={
                "from": ",".join(to_str(keywords)),
                "to": to_str(changeto),
                "num": changed_objects,
            },
        )
        if changed_objects:
            msg_type = "info"
        else:
            msg_type = "warning"

        return self.doReturn(msg, msg_type, field=field)

    def deleteKeywords(self, keywords, field):
        pkm = getToolByName(self.context, "portal_keyword_manager")
        deleted_objects = pkm.delete(keywords, context=self.context, indexName=field)
        msg = _(
            "msg_deleted_keywords",
            default=u"Deleted ${keywords} for ${num} object(s).",
            mapping={"keywords": ",".join(to_str(keywords)), "num": deleted_objects},
        )

        if deleted_objects:
            msg_type = "info"
        else:
            msg_type = "warning"

        return self.doReturn(msg, msg_type, field=field)

    def doReturn(self, message="", msg_type="", field=""):
        """
        set the message and return
        """
        if message and msg_type:
            pu = getToolByName(self.context, "plone_utils")
            pu.addPortalMessage(safe_encode(message), type=msg_type)

        logger.info(safe_encode(message))
        portal_url = self.context.portal_url()
        url = "%s/prefs_keywords_view" % portal_url
        if field:
            url = "%s?field=%s" % (url, field)

        self.request.RESPONSE.redirect(url)


class KeywordsSearchResults(BrowserView):

    def __call__(self):
        items = []
        try:
            per_page = int(self.request.form.get('perPage'))
        except ValueError:
            per_page = 10
        try:
            page = int(self.request.form.get('page'))
        except ValueError:
            page = 1

        search_string = self.request.form.get('s')
        field = self.request.form.get('field')

        results = self.results(search_string, index_name=field)
        portal_url = self.context.portal_url()

        for result in results:
            items.append({'id': result,
                          'title': result,
                          'description': '',
                          'state': "keyword",
                          'url': "%s/prefs_keywords_view?field=%s&s=%s" % (portal_url, field, search_string),
                          })

        self.request.response.setHeader("Content-type", "application/json")

        return json.dumps({
            'total': len(results),
            'items': items
        })


    def results(self, search_string, index_name):
        pkm = getToolByName(self.context, "portal_keyword_manager")

        num = 100
        score = 0.6
        all_keywords = pkm.getKeywords(context=self.context, indexName=index_name)
        return pkm.getScoredMatches(search_string, all_keywords, num, score, context=self.context)




