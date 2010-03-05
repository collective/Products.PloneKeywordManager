###
# PloneKeywordManager-Installer
###

from StringIO import StringIO
from Products.CMFCore.utils import getToolByName
from Products.CMFCore.DirectoryView import addDirectoryViews
#from Products.CMFCore.permissions import ManagePortal
from Products.PloneKeywordManager import cmf_keyword_manager_globals, config
from OFS.ObjectManager import BadRequestException

# The Configlet category. Normally this should be "Products" for third-party
# Products, but since this tool is actually just extending Plone somewhat,
# it makes more sense to use the Plone category. Don't copy it if you don't
# have such an app.

configlets = (
              {'id': 'keywordmanager',
               'name': 'Keyword Manager',
               'action': 'string:${portal_url}/prefs_keywords_view',
               'category': 'Products',
               'appId': 'PloneKeywordManager',
               'permission': config.MANAGE_KEYWORDS_PERMISSION,
               'imageUrl': 'book_icon.gif'},
              )


def install_subskin(self, out, skin_name, globals):
    skinstool=getToolByName(self, 'portal_skins')
    if skin_name not in skinstool.objectIds():
        addDirectoryViews(skinstool, 'skins', globals)

    for skinName in skinstool.getSkinSelections():
        path = skinstool.getSkinPath(skinName)
        path = [i.strip() for i in path.split(',')]
        try:
            if skin_name not in path:
                path.insert(path.index('custom')+1, skin_name)
        except ValueError:
            if skin_name not in path:
                path.append(skin_name)

        path = ','.join(path)
        skinstool.addSkinSelection(skinName, path)


def addConfiglets(self, out):
    configTool = getToolByName(self, 'portal_controlpanel', None)
    if configTool:
        for conf in configlets:
            out.write('Adding configlet %s\n' % conf['id'])
            configTool.registerConfiglet(**conf)


def install(self):
    out = StringIO()
    print >>out, "Installing PloneKeywordManager."

    install_subskin(self, out, 'keyword_manager', cmf_keyword_manager_globals)
    print >>out, "Added skin."

    addConfiglets(self, out)
    print >>out, "Added control panel."

    try:
        self.manage_addProduct['PloneKeywordManager']. \
            manage_addTool(config.TOOL_NAME)
    except BadRequestException:
        print >>out, "Already installed, not adding tool to portal root."

    print >>out, "Done."


    return out.getvalue()


# Uninstall methods
def removeConfiglets(self, out):
    configTool = getToolByName(self, 'portal_controlpanel', None)
    if configTool:
        for conf in configlets:
            out.write('Removing configlet %s\n' % conf['id'])
            configTool.unregisterConfiglet(conf['id'])

# The uninstall is used by the CMFQuickInstaller for uninstalling.
# CMFQuickInstaller uninstalls skins.
def uninstall(self):
    out=StringIO()
    removeConfiglets(self, out)
    return out.getvalue()
