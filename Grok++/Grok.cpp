#include "Grok.h"

const regex Grok::_grokNamedExpression = regex("%\\{(\\w+):(\\w+)(?::\\w+)?\\}");	// all named expressions 
const regex Grok::_grokUnnamedExpression = regex("%\\{(\\w+)\\}");					// all unmaned expressions

const regex Grok::_grokRegexWithType = regex("(%\\{(\\w+):(\\w+):(\\w+)\\})*?");	// typed expressions

