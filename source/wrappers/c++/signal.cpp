#include "signal.hpp"

#include <boost/format.hpp>
#include <uda/client.h>

void uda::Signal::put() const
{
    // Open file
    const char* fmt = "putdata::open(filename='%1%.nc', conventions='FUSION', class='%2%', shot='%3%', pass='%4%', "
                      "comment='%5%', /create)";
    std::string query = (boost::format(fmt) % alias_ % signal_class_ % shot_ % pass_ % comment_).str();
    idamPutAPI(query.c_str(), NULL);

    idamPutAPI("", NULL);

    idamPutAPI("", NULL);

    idamPutAPI("", NULL);

    idamPutAPI("", NULL);

    idamPutAPI("", NULL);

    idamPutAPI("", NULL);
}
