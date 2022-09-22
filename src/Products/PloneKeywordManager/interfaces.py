# Copyright (c) 2005 gocept gmbh & co. kg
# See also LICENSE.txt
# $Id$

from zope.interface import Interface


class IKeywordManager(Interface):
    """A utility that allows to manage keywords"""

    def change(old_keywords, new_keyword):
        """Updates all objects using the old_keywords.

        Objects using the old_keywords will be using the new_keywords
        afterwards.
        """

    def delete(keywords):
        """Removes the keywords from all objects using it."""
