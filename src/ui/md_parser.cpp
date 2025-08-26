#include "md_parser.hpp"

#include <sstream>
#include <md4c.h>
#include <md4c-html.h>

static void mdHtmlWrite(const MD_CHAR *str, MD_SIZE size, void *ud) {
    std::stringstream *outStream = (std::stringstream*) ud;
    outStream->write(str, size);
}

std::string parseMarkdownToHTML(const std::string &src) {
    std::stringstream htmlOutput;
    md_html(
        src.c_str(), src.size(), mdHtmlWrite, &htmlOutput,
        MD_FLAG_PERMISSIVEURLAUTOLINKS | MD_FLAG_PERMISSIVEWWWAUTOLINKS, 0);
    return htmlOutput.str();
}