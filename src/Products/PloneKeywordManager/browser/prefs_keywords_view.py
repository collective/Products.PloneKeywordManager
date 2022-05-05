from plone import api
from Products.CMFPlone.PloneBatch import Batch
from Products.Five import BrowserView
from Products.Five.browser.pagetemplatefile import ViewPageTemplateFile
from Products.PloneKeywordManager import keywordmanagerMessageFactory as _
from Products.PloneKeywordManager.compat import to_str
from ZTUtils import make_query

import json
import logging


logger = logging.getLogger("Products.PloneKeywordManager")


class PrefsKeywordsView(BrowserView):
    """
    A view to manage the keywords in the portal
    """

    template = ViewPageTemplateFile("prefs_keywords_view.pt")

    def __init__(self, context, request):
        super().__init__(context, request)
        self.pkm = api.portal.get_tool("portal_keyword_manager")

    def __call__(self):
        self.is_plone_5 = True
        if not self.request.form.get(
            "form.button.Merge", ""
        ) and not self.request.form.get("form.button.Delete", ""):
            return self.template({})

        keywords = self.request.get("keywords", None)
        field = self.request.get("field", None)

        if not keywords:
            message = _("Please select at least one keyword")
            return self.doReturn(message, "error")

        if not field or field not in self.pkm.getKeywordIndexes():
            message = _("Please select a valid keyword field")
            return self.doReturn(message, "error")

        if "form.button.Merge" in self.request.form:
            # We should assume there is a 'changeto' filled
            changeto = self.request.get("changeto", None)
            if not changeto:
                message = _("Please provide a new term")
                return self.doReturn(message, "error")

            return self.changeKeywords(keywords, changeto, field)

        if "form.button.Delete" in self.request.form:
            return self.deleteKeywords(keywords, field)

    def getNavrootUrl(self):
        return api.portal.get_navigation_root(self.context).absolute_url()

    def getKeywords(self, indexName, b_start=0, b_size=30):
        """
        :param indexName the name of the index we want to get all keywords for.
        :param b_start: Batching support - page to start from
        :param b_size: Batching support - size of page
        :return: a Products.CMFPlone Batch object containing the entire list of keywords.
        """
        search_string = self.request.get("s", None)

        if not search_string:
            keywords = self.pkm.getKeywords(indexName=indexName)
        else:
            all = self.pkm.getKeywords(indexName=indexName)
            max_results = 100000  # I don't want to limit the results here... this is simply a big number.
            score = 0.5
            keywords = self.pkm.getScoredMatches(
                search_string, all, max_results, score, context=self.context
            )

        return Batch(keywords, b_size, b_start)

    def getNumObjects(self, keyword, indexName):
        """
        return the number of indexed objects with the specificed keyword
        :param keyword: string
        :return: int
        """
        return self.pkm.getKeywordLength(keyword, indexName)

    def getKeywordIndexes(self):
        return self.pkm.getKeywordIndexes()

    def getScoredMatches(self, keyword, batch, num_similar, score):
        return self.pkm.getScoredMatches(
            keyword, batch, num_similar, score, context=self.context
        )

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
            we remove these keywords from the field 'subject'
            we then add the keyword 'Foo' (if it didn't alreay exist) to the subject field.
            we save all those objects.

          we should also rebuild the index, but hey... that's work.
        """
        changed_objects = self.pkm.change(
            keywords, changeto, context=self.context, indexName=field
        )
        msg = _(
            "msg_changed_keywords",
            default="Changed ${from} to ${to} for ${num} object(s).",
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

        return self.doReturn(msg, msg_type)

    def deleteKeywords(self, keywords, field):
        deleted_objects = self.pkm.delete(
            keywords, context=self.context, indexName=field
        )
        msg = _(
            "msg_deleted_keywords",
            default="Deleted ${keywords} for ${num} object(s).",
            mapping={"keywords": ",".join(to_str(keywords)), "num": deleted_objects},
        )

        if deleted_objects:
            msg_type = "info"
        else:
            msg_type = "warning"

        return self.doReturn(msg, msg_type)

    def doReturn(self, message="", msg_type=""):
        """
        set the message and return
        """
        if message and msg_type:
            pu = api.portal.get_tool("plone_utils")
            pu.addPortalMessage(message, type=msg_type)

        logger.info(self.context.translate(message))
        navroot_url = api.portal.get_navigation_root(self.context).absolute_url()
        url = f"{navroot_url}/prefs_keywords_view"

        query = dict()
        if self.request.get("field", False):
            query["field"] = self.request["field"]
        if self.request.get("s", False):
            query["s"] = self.request["s"]
        if self.request.get("b_start", False):
            query["b_start"] = self.request["b_start"]

        self.request.RESPONSE.redirect(f"{url}?{make_query(**query)}")


class KeywordsSearchResults(BrowserView):
    def __call__(self):
        items = []
        search_string = self.request.form.get("s")
        field = self.request.form.get("field")

        results = self.results(search_string, index_name=field)
        navroot_url = api.portal.get_navigation_root(self.context).absolute_url()

        for result in results:
            items.append(
                {
                    "id": result,
                    "title": result,
                    "description": "",
                    "state": "keyword",
                    "url": f"{navroot_url}/prefs_keywords_view?field={field}&s={result}",
                }
            )

        self.request.response.setHeader("Content-type", "application/json")

        return json.dumps({"total": len(results), "items": items})

    def results(self, search_string, index_name):
        pkm = api.portal.get_tool("portal_keyword_manager")

        num = 100
        score = 0.6
        all_keywords = pkm.getKeywords(indexName=index_name)
        return pkm.getScoredMatches(
            search_string, all_keywords, num, score, context=self.context
        )
