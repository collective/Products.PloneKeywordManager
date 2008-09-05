## Script (Python) "prefs_keywords_action_change"
##bind container=container
##bind context=context
##bind namespace=
##bind script=script
##bind subpath=traverse_subpath
##parameters=keywords, changeto
##title=
##
from Products.CMFCore.utils import getToolByName
pkm = getToolByName(context, 'portal_keyword_manager')
changed_objects = pkm.change(keywords, changeto, context=context.aq_inner)

msg ="Changed %s to %s for %d object(s)." % (','.join(keywords),
                                             changeto, changed_objects)

context.REQUEST.RESPONSE.redirect('prefs_keywords_view' + 
                                  '?portal_status_message=%s' % (msg,))
