/*
 * The module provides two functions: UrlEncode() and UrlDecode() which
 * perform encoding / decoding of strings according to the percent-encoding
 * scheme, also known as URL-encoding.
 *
 * RFC 3986 (section 2) and RFC 1738 (section 2.2) comment the needs
 * and issues of encoding regarding URLs.
 *
 * The application/x-www-form-urlencoded content type encoding is discussed
 * here:
 *    http://www.w3.org/TR/html401/interact/forms.html#h-17.13.4
 *
 * Author:    Yassen Damyanov
 * Created:   2012-11-13
 * Changelog: See bottom of file
 *
 * Author:    Mihail Grigorov
 * Changed:   2018-12-11
 * ChangeLog: MG, 2018-12-11 Adds additional parameter (srclen) 
 */

#include <string.h>

#ifdef TESTING
#include <iostream>
#include <assert.h>
#else
#include <ctype.h>
#endif

#include "url-codec.h"

// application/x-www-form-urlencoded content type requires space encoded into '+':
//# define SPACE_TO_PLUS

using namespace std;

const char hexchars[]		= "0123456789ABCDEFabcdef";
#ifdef SPACE_TO_PLUS
const char unsafe[]			= "%:/?#[]@!$&'\"()*+,;=";
#else
const char unsafe[]			= " %:/?#[]@!$&'\"()*+,;=";
#endif


/*
 * Helper for UrlEncode(). Returns true if given char is regarded unsafe
 * for transmission (if found in const char unsafe[]) and needs to be
 * percent-encoded, false otherwise.
 *
 * Adjust unsafe[] to fit the needs of your application. Do not forget
 * that space needs to be excluded from unsafe[] if special encoding
 * rules apply for it (e.g. encoding it as '+').
 */
inline bool is_unsafe(char ch) {
	for (int i=0; unsafe[i]; i++)
		if (unsafe[i] == ch)  return true;
	return false;
}

/*
 * Helper for UrlDecode(). Returns the hex digit order for given char:
 * 0 for '0', 1 for '1' and so on ... 9 for '9', 10 for 'A' or 'a',
 * ... 15 for 'F' or 'f';  or it returns -1 if given char is not a valid
 * hex digit.
 */
inline int hex_digit_order(char ch) {
	if (ch >= '0' && ch <= '9')  return ch -'0';
	ch = toupper(ch);
	if (ch >= 'A' && ch <= 'F')  return ch -'A' + 10;
	return -1;
}

/**
 * Decodes given src string into the dest char* buffer according to the
 * percent-encoding schema described in
 *   http://en.wikipedia.org/wiki/Percent-encoding
 *
 * As far as there are different implementations, the exact encoding scheme
 * is tunable via the char [] unsafe variable above, as well as the
 * SPACE_TO_PLUS macro.
 *
 * Returns the exact length of the decoded string in dest. In case dest is
 * not long enough to cope with the whole decode-resulting string, 0 is
 * returned. If src is not correctly encoded,  URL_DECODE_ERR macro is
 *  returned.
 */
size_t UrlDecode(const char* src, size_t srclen, char* dest, size_t dstlen) {
	size_t i, di = 0;
	for (i = 0; src[i] && i < srclen; i++) {
		if (di +1 > dstlen)  return 0;	// dest not big enough
		char ch = src[i];
		int high, low;
		switch (ch) {
			case '%':
				high = hex_digit_order(src[i +1]);
				low  = hex_digit_order(src[i +2]);
				if (-1 == high || -1 == low )
					return URL_DECODE_ERR;	// incorrect url-encoding
				dest[di ++] = (char) (high << 4 | low);
				i += 2;
				break;
			// we always try to decode '+' as space (' '):
			case '+':
				dest [di ++] = ' ';
				break;
			default:
				dest[di ++] = ch;
				break;
		}
	}
	dest[di] = '\0';
	return di;
}


/**
 * Encodes given src string into the dest char* buffer according to the
 * percent-encoding schema described in
 *   http://en.wikipedia.org/wiki/Percent-encoding
 *
 * As far as there are different implementations, this function should
 * work for all (most?) cases, including space encoded as '+' or '%20'.
 *
 * Returns the exact length of the decoded string in dest. In case dest is
 * not long enough to cope with the whole encode-resulting string, 0 is
 * returned.
 */
size_t UrlEncode(const char* src, size_t srclen, char* dst, size_t dlen) {
	size_t i, di = 0;
	for (i = 0; src[i] && i < srclen; i++) {
		char ch = src[i];
		if (is_unsafe(ch)) {
			if (di +3 > dlen)  return 0;	// dst not long enough
			dst[di ++] = '%';
			dst[di ++] = hexchars[ch / 16];
			dst[di ++] = hexchars[ch % 16];
		// We only encode space as '+' if SPACE_TO_PLUS is defined.
		// Most URL encoders do NOT encode space as '+' by default.
#ifdef SPACE_TO_PLUS
		} else if (' ' == ch) {
			if (di +1 > dlen)  return 0;	// dst not long enough
			dst[di ++] = '+';
#endif
		} else {
			if (di +1 > dlen)  return 0;	// dst not long enough
			dst[di ++] = ch;
		}
	}
	dst[di] = '\0';
	return di;
}


#ifdef TESTING
/*
 * Tests for each of the public and helper functions.
 *
 * Compile and run on TESTING macro defined (-DTESTING).
 * Define VERBOSE to see the sample strings dumped on stdout.
 * Something like this will do:
 *
 *   $ clear && g++ -DTESTING -DVERBOSE -o test url-codec.cpp && ./test && rm test
 */

void _test_is_unsafe() {
	cout << "_test_is_unsafe(): ";
	assert(is_unsafe('%')); assert(is_unsafe('+')); assert(is_unsafe('&'));
	assert(is_unsafe('='));	assert(is_unsafe('/'));
#ifndef SPACE_TO_PLUS
	assert(is_unsafe(' '));
#endif
	assert(!is_unsafe('.')); assert(!is_unsafe('-')); assert(!is_unsafe('_'));
	cout << "OK!" << endl;
}

void _test_isdigit() {
	cout << "_test_isdigit(): ";
	assert(isdigit('0'));	assert(isdigit('1'));	assert(isdigit('2'));	assert(isdigit('3'));
	assert(isdigit('4'));	assert(isdigit('5'));	assert(isdigit('6'));	assert(isdigit('7'));
	assert(isdigit('8'));	assert(isdigit('9'));	assert(!isdigit('A'));	assert(!isdigit('F'));
	assert(!isdigit('a'));	assert(!isdigit('f'));	assert(!isdigit('X'));
	cout << "OK!" << endl;
}

void _test_hex_digit_order() {
	cout << "_test_hex_digit_order(): ";
	assert(-1 == hex_digit_order('x'));	assert(-1 == hex_digit_order('X')); assert(-1 == hex_digit_order(' '));
	assert(-1 == hex_digit_order('/'));	assert(-1 == hex_digit_order('.')); assert(-1 == hex_digit_order('_'));
	assert(0 == hex_digit_order('0'));	assert(1 == hex_digit_order('1'));	assert(2 == hex_digit_order('2'));
	assert(3 == hex_digit_order('3'));	assert(4 == hex_digit_order('4'));	assert(5 == hex_digit_order('5'));
	assert(6 == hex_digit_order('6'));	assert(7 == hex_digit_order('7'));	assert(8 == hex_digit_order('8'));
	assert(9 == hex_digit_order('9'));	assert(10 == hex_digit_order('A'));	assert(10 == hex_digit_order('a'));
	assert(11 == hex_digit_order('B'));	assert(11 == hex_digit_order('b'));	assert(12 == hex_digit_order('C'));
	assert(12 == hex_digit_order('c'));	assert(13 == hex_digit_order('D'));	assert(13 == hex_digit_order('d'));
	assert(14 == hex_digit_order('E'));	assert(14 == hex_digit_order('e'));	assert(15 == hex_digit_order('F'));
	assert(15 == hex_digit_order('f'));
	assert(-1 == hex_digit_order('G'));	assert(-1 == hex_digit_order('g'));
	cout << "OK!" << endl;
}

void _do_encode_decode_test(char* src, char* dst, char* exp) {
	size_t slen  = strlen(src);
	size_t dlen  = strlen(dst);
	size_t len = UrlEncode(src, dst, dlen);
	assert(len > 0 && len < URL_DECODE_ERR);
	size_t dlen_new  = strlen(dst);
	assert(len == dlen_new);
#ifdef VERBOSE
	cout << endl;
	cout << "src: " << src << endl;
	cout << "exp: " << exp << endl;
	cout << "dst: " << dst << endl;
#endif
	assert(0 == strcmp(exp, dst));
	char dst2[dlen_new + 1];
	strncpy(dst2, dst, dlen_new);
	//for (int i=0; dst[i]; i++) dst2[i] = dst[i];
	dst2[dlen_new] = '\0';
	assert(0 == strcmp(dst2, dst));
	size_t dlen2 = strlen(dst2);
	assert(dlen2 == dlen_new);
	size_t len2 = UrlDecode(dst, dst2, dlen2);
#ifdef VERBOSE
	cout << "src: " << src << endl;
	cout << "new: " << dst2 << endl;
#endif
	assert(len2 == slen);
	assert(0 == strcmp(src, dst2));
}

void _test_encoding_decoding() {
	cout << "_test_encoding_decoding(): ";
	char dst[4097];
	char* src;
	char* exp;
	// test #1
	for (int i=0; i < 4096; i++) dst[i] = ' '; dst[4096] = '\0';	// big enough buffer
	src = "%&=55463B4C7037C55E42%&=85A928198E5 9997A36F055/0003/00000040/2807111244/03TZ740000498#4999.99";
#ifdef SPACE_TO_PLUS
	exp = "%25%26%3D55463B4C7037C55E42%25%26%3D85A928198E5+9997A36F055%2F0003%2F00000040%2F2807111244%2F03TZ740000498%234999.99";
#else
	exp = "%25%26%3D55463B4C7037C55E42%25%26%3D85A928198E5%209997A36F055%2F0003%2F00000040%2F2807111244%2F03TZ740000498%234999.99";
#endif
	_do_encode_decode_test(src, dst, exp);
	// test #2
	for (int i=0; i < 4096; i++) dst[i] = ' '; dst[4096] = '\0';	// big enough buffer
	src  = "$99.99 and 10% tax";
#ifdef SPACE_TO_PLUS
	exp  = "%2499.99+and+10%25+tax";
#else
	exp  = "%2499.99%20and%2010%25%20tax";
#endif
	_do_encode_decode_test(src, dst, exp);
	// test #3: exact length of dst string:
	for (int i=0; i < 4096; i++) dst[i] = ' '; dst[4096] = '\0';	// big enough buffer
	src  = "A";
	exp  = "A";
	_do_encode_decode_test(src, dst, exp);

	// test #4: exact length of dst string, again:
	for (int i=0; i < 4096; i++) dst[i] = ' '; dst[4096] = '\0';	// big enough buffer
	src  = "+&";
	exp  = "%2B%26";
	_do_encode_decode_test(src, dst, exp);

	// test #4: insufficient length of dst string:
//	src  = "+";
//	exp  = "A";
//	_do_encode_decode_test(src, dst, exp);

	cout << "OK!" << endl;
	return;
}

void _do_tests() {
	_test_isdigit();
	_test_is_unsafe();
	_test_hex_digit_order();
	_test_encoding_decoding();
	cout << "ALL TESTS OK." << endl;
}

#endif

#ifdef TESTING
int main() {
	cout << "TESTING      : true" << endl;
#ifdef SPACE_TO_PLUS
	cout << "SPACE_TO_PLUS: true" << endl;
#endif
	_do_tests();
	return 0;
}
//#else
//int main() {
//	cout << "Url Encode-Decode stuff. Use the source, Luke! ;)   (or -DTESTING to compile the tests)" << endl;
//	return 0;
//}
#endif

/*
 * CHANGELOG
 * YD, 2012-11-14: removed is_hex_digit() and hex_to_char(), combining the two in a more efficient hex_digit_order()
 */
// eof //
