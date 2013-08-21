# -*- coding: utf-8 -*-
from Products.PloneKeywordManager import logger


default_profile = 'profile-Products.PloneKeywordManager:default'
uninstall_profile = 'profile-Products.PloneKeywordManager:uninstall'


def to_3(context):
    """
    remove unused skins and add browserlayer
    """
    logger.info('Upgrading Products.PloneKeywordManager to version 3')
    context.runImportStepFromProfile(uninstall_profile, 'skins')
    context.runImportStepFromProfile(default_profile, 'rolemap')
    context.runImportStepFromProfile(default_profile, 'browserlayer')
