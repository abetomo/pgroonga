#include "pgroonga.h"

#include "pgrn-column-name.h"
#include "pgrn-groonga.h"

#include <postgres.h>

static const char *ENCODED_CHARACTER_FORMAT = "@%05x";
static const int ENCODED_CHARACTER_LENGTH = 6;

static bool
PGrnColumnNameIsUsableCharacterASCII(char character)
{
	return (character == '_' || ('0' <= character && character <= '9') ||
			('A' <= character && character <= 'Z') ||
			('a' <= character && character <= 'z'));
}

static void
PGrnColumnNameEncodeCharacterUTF8(const char *utf8Character, char *encodedName)
{
	pg_wchar codepoint;
	codepoint = utf8_to_unicode((const unsigned char *) utf8Character);
	snprintf(encodedName,
			 ENCODED_CHARACTER_LENGTH + 1,
			 ENCODED_CHARACTER_FORMAT,
			 codepoint);
}

static void
checkSize(size_t size, const char *tag)
{
	if (size >= GRN_TABLE_MAX_KEY_SIZE)
		PGrnCheckRC(GRN_INVALID_ARGUMENT,
					"%s encoded column name >= %d",
					tag,
					GRN_TABLE_MAX_KEY_SIZE);
}

static size_t
PGrnColumnNameEncodeUTF8WithSize(const char *name,
								 size_t nameSize,
								 char *encodedName)
{
	const char *tag = "[column-name][encode][utf8]";
	const char *current;
	const char *end;
	char *encodedCurrent;
	size_t encodedNameSize = 0;

	current = name;
	end = name + nameSize;
	encodedCurrent = encodedName;
	while (current < end)
	{
		int length;

		length = pg_mblen(current);

		if (length == 1 && PGrnColumnNameIsUsableCharacterASCII(*current) &&
			!(*current == '_' && current == name))
		{
			checkSize(encodedNameSize + length + 1, tag);
			*encodedCurrent++ = *current;
			encodedNameSize++;
		}
		else
		{
			checkSize(encodedNameSize + ENCODED_CHARACTER_LENGTH + 1, tag);
			PGrnColumnNameEncodeCharacterUTF8(current, encodedCurrent);
			encodedCurrent += ENCODED_CHARACTER_LENGTH;
			encodedNameSize += ENCODED_CHARACTER_LENGTH;
		}

		current += length;
	}

	*encodedCurrent = '\0';

	return encodedNameSize;
}

size_t
PGrnColumnNameEncode(const char *name, char *encodedName)
{
	return PGrnColumnNameEncodeWithSize(name, strlen(name), encodedName);
}

size_t
PGrnColumnNameEncodeWithSize(const char *name,
							 size_t nameSize,
							 char *encodedName)
{
	const char *tag = "[column-name][encode]";
	const char *current;
	const char *end;
	char *encodedCurrent;
	size_t encodedNameSize = 0;

	if (GetDatabaseEncoding() == PG_UTF8)
		return PGrnColumnNameEncodeUTF8WithSize(name, nameSize, encodedName);

	current = name;
	end = name + nameSize;
	encodedCurrent = encodedName;
	while (current < end)
	{
		int length;

		length = pg_mblen(current);
		if (length != 1)
			PGrnCheckRC(GRN_FUNCTION_NOT_IMPLEMENTED,
						"%s multibyte character isn't supported "
						"for column name except UTF-8 encoding: <%s>(%s)",
						tag,
						name,
						GetDatabaseEncodingName());

		if (PGrnColumnNameIsUsableCharacterASCII(*current) &&
			!(*current == '_' && current == name))
		{
			checkSize(encodedNameSize + length + 1, tag);
			*encodedCurrent++ = *current;
			encodedNameSize++;
		}
		else
		{
			checkSize(encodedNameSize + ENCODED_CHARACTER_LENGTH + 1, tag);
			PGrnColumnNameEncodeCharacterUTF8(current, encodedCurrent);
			encodedCurrent += ENCODED_CHARACTER_LENGTH;
			encodedNameSize += ENCODED_CHARACTER_LENGTH;
		}

		current++;
	}

	*encodedCurrent = '\0';

	return encodedNameSize;
}

size_t
PGrnColumnNameDecode(const char *encodedName, char *name)
{
	/* TODO */
	return 0;
}
