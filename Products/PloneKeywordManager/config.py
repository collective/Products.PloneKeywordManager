PROJECTNAME = 'PloneKeywordManager'

MANAGE_KEYWORDS_PERMISSION = "Manage Keywords"

TOOL_NAME = "Plone Keyword Manager Tool"

#Meta type of the keyword indexes. If you're one of those crazy people that use
#custom indexes, you'll want to update this.
META_TYPE = 'KeywordIndex'

#indexes of META_TYPE we know we don't want to manage, because bad things(tm) will happen
IGNORE_INDEXES = [
    'object_provides',
    'allowedRolesAndUsers',
    'getRawRelatedItems',
    'getEventType',
]

#A list of indexes that should always be reindexed when merging or deleting
#keywords on objects. Most people won't need this.
ALWAYS_REINDEX = (
    #'Subject',
)
