## Script (Python) "prefs_keywords_action_change"
##bind container=container
##bind context=context
##bind namespace=
##bind script=script
##bind state=state
##bind subpath=traverse_subpath
##parameters=keywords, changeto, field
##title=
##
from Products.CMFPlone import PloneMessageFactory as _
from Products.CMFCore.utils import getToolByName

pkm = getToolByName(context, 'portal_keyword_manager')
changed_objects = pkm.change(keywords, changeto, context=context.aq_inner, indexName=field)

msg = _('msg_changed_keywords', default=u"Changed ${from} to ${to} for ${num} object(s).",
       mapping={'from': ','.join(keywords).decode('utf-8'),
                'to': changeto.decode('utf-8'),
                'num': changed_objects})

state.setNextAction('redirect_to:string:prefs_keywords_view?field=%s' % field)

context.plone_utils.addPortalMessage(msg)

return state
