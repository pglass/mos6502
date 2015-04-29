#ifndef NULLSTREAM_H
#define NULLSTREAM_H

/* The stream equivalent of /dev/null
 * Borrowed from: http://stackoverflow.com/a/6240980
 */
template<typename Ch, typename Traits = std::char_traits<Ch> >
struct basic_nullbuf : std::basic_streambuf<Ch, Traits> {
     typedef std::basic_streambuf<Ch, Traits> base_type;
     typedef typename base_type::int_type int_type;
     typedef typename base_type::traits_type traits_type;

     virtual int_type overflow(int_type c) {
         return traits_type::not_eof(c);
     }
};

typedef basic_nullbuf<char> nullbuf;

static nullbuf _NULLBUF;

/* Use this instead of constructing one all the time.
 * This can be used as a default arg for a &std::ostream.
 */
static std::ostream NULLSTREAM(&_NULLBUF);

#endif // NULLSTREAM_H
