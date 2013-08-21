from Products.Five import BrowserView
from Products.Five.browser.pagetemplatefile import ViewPageTemplateFile
from Products.PloneKeywordManager import keywordmanagerMessageFactory as _
from Products.CMFCore.utils import getToolByName


class PrefsKeywordsView(BrowserView):
    """
    A view to manage the keywords in the portal
    """

    template = ViewPageTemplateFile('prefs_keywords_view.pt')

    def __call__(self):
        if not self.request.form.get('form.button.Merge', '') and not self.request.form.get('form.button.Delete', ''):
            return self.template({})
        pkm = getToolByName(self.context, 'portal_keyword_manager')

        keywords = self.request.get('keywords', None)
        field = self.request.get('field', None)

        if not keywords:
            message = _(u'Please select at least one keyword')
            return self.doReturn(message, 'error')

        if not field or field not in pkm.getKeywordIndexes():
            message = _(u'Please select a valid keyword field')
            return self.doReturn(message, 'error')

        if 'form.button.Merge' in self.request.form:
            # We should assume there is a 'changeto' filled
            changeto = self.request.get('changeto', None)
            if not changeto:
                message = _(u'Please provide a new term')
                return self.doReturn(message, 'error')

            return self.changeKeywords(keywords, changeto, field)

        if 'form.button.Delete' in self.request.form:
            return self.deleteKeywords(keywords, field)

    def changeKeywords(self, keywords, changeto, field):
        """
        """
        pkm = getToolByName(self.context, 'portal_keyword_manager')
        changed_objects = pkm.change(keywords, changeto, context=self.context, indexName=field)

        msg = _('msg_changed_keywords', default=u"Changed ${from} to ${to} for ${num} object(s).",
                mapping={'from': ','.join(keywords).decode('utf-8'),
                         'to': changeto.decode('utf-8'),
                         'num': changed_objects})

        if changed_objects:
            msg_type = 'info'
        else:
            msg_type = 'warning'

        return self.doReturn(msg, msg_type)

    def deleteKeywords(self, keywords, field):
        pkm = getToolByName(self.context, "portal_keyword_manager")
        deleted_objects = pkm.delete(keywords, context=self.context, indexName=field)

        msg = _('msg_deleted_keywords', default=u"Deleted ${keywords} for ${num} object(s).",
                mapping={'keywords': ','.join(keywords).decode('utf-8'),
                         'num': deleted_objects})

        if deleted_objects:
            msg_type = 'info'
        else:
            msg_type = 'warning'

        return self.doReturn(msg, msg_type)

    def doReturn(self, message='', type=''):
        """
        set the message and return
        """
        if message and type:
            pu = getToolByName(self.context, "plone_utils")
            pu.addPortalMessage(message, type=type)
        portal_url = self.context.portal_url()
        self.request.RESPONSE.redirect("%s/prefs_keywords_view" % portal_url)
