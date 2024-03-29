/* Yarin Knafo - 205580624
 * Maor Cohen - 301729414 */

#include "Query.h"
#include "TextQuery.h"
#include <memory>
#include <set>
#include <algorithm>
#include <iostream>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <regex>
using namespace std;
////////////////////////////////////////////////////////////////////////////////
shared_ptr<QueryBase> QueryBase::factory(const string &s) {

    regex wordRegexPattern(R"(^\s*([\w']+)\s*$)");
    regex notRegexPattern(R"(^\s*NOT\s+([\w']+)\s*$)");
    regex andRegexPattern(R"(^\s*([\w']+)\s+AND\s+([\w']+)\s*$)");
    regex orRegexPattern(R"(^\s*([\w']+)\s+OR\s+([\w']+)\s*$)");
    regex nRegexPattern(R"(^\s*([\w']+)\s+(\d+)\s+([\w']+)\s*$)");

    if (regex_match(s, wordRegexPattern)) {
        return shared_ptr<QueryBase>(new WordQuery(s));
    }

    else if (regex_match(s, notRegexPattern)) {
        istringstream iss(s);
        vector<string> results((istream_iterator<string>(iss)), istream_iterator<string>());
        return shared_ptr<QueryBase>(new NotQuery(results[1]));
    }

    else if (regex_match(s, andRegexPattern)) {
        istringstream iss(s);
        vector<string> results((istream_iterator<string>(iss)),istream_iterator<string>());
        return shared_ptr<QueryBase>(new AndQuery(results[0], results[2]));
    }

    else if (regex_match(s, orRegexPattern )) {
        istringstream iss(s);
        vector<string> results((istream_iterator<string>(iss)), istream_iterator<string>());
        return shared_ptr<QueryBase>(new OrQuery(results[0], results[2]));
    }
    else if (regex_match(s, nRegexPattern)) {
        istringstream iss(s);
        vector<string> results((istream_iterator<string>(iss)), istream_iterator<string>());
        int d = stoi(results[1]);
        return shared_ptr<QueryBase>(new NQuery(results[0], results[2], d));
    }
    else {
        throw invalid_argument("Unrecognized search");
    }
}
QueryResult NotQuery::eval(const TextQuery &text) const {
    QueryResult result = text.query(query_word);
    auto ret_lines = std::make_shared<std::set<line_no>>();
    auto beg = result.begin(), end = result.end();
    auto sz = result.get_file()->size();

    for (size_t n = 0; n != sz; ++n) {
        if (beg == end || *beg != n)
            ret_lines->insert(n);
        else if (beg != end)
            ++beg;
    }
    return QueryResult(rep(), ret_lines, result.get_file());
}
////////////////////////////////////////////////////////////////////////////////


QueryResult AndQuery::eval (const TextQuery& text) const
{
    QueryResult left_result = text.query(left_query);
    QueryResult right_result = text.query(right_query);

    auto ret_lines = make_shared<set<line_no>>();
    set_intersection(left_result.begin(), left_result.end(),
                          right_result.begin(), right_result.end(),
                          inserter(*ret_lines, ret_lines->begin()));

    return QueryResult(rep(), ret_lines, left_result.get_file());
}

QueryResult OrQuery::eval(const TextQuery &text) const
{
    QueryResult left_result = text.query(left_query);
    QueryResult right_result = text.query(right_query);

    auto ret_lines =
            make_shared<set<line_no>>(left_result.begin(), left_result.end());

    ret_lines->insert(right_result.begin(), right_result.end());

    return QueryResult(rep(), ret_lines, left_result.get_file());
}
/////////////////////////////////////////////////////////
QueryResult NQuery::eval(const TextQuery &text) const {
    QueryResult result = AndQuery::eval(text);

    auto it= result.begin();
    auto ret_lines = std::make_shared<std::set<line_no>>();

    regex words_regex("[\\w']+");
    regex regex_left_to_right("(.)*" + left_query + " ([\\w']+ ){0," + to_string(dist) +"}" + right_query + "(.)*");
    regex regex_right_to_left("(.)*" + right_query + " ([\\w']+ ){0," + to_string(dist) +"}" + left_query + "(.)*");

    for (; it != result.end(); ++it) {
        string line = result.get_file()->at(*it);
        string newLine = "";
        auto start = sregex_iterator(line.begin(), line.end(), words_regex);
        auto end = sregex_iterator();
        for (sregex_iterator i = start; i != end; ++i) {
            smatch match = *i;
            string match_str = match.str();
            newLine = newLine + match_str + " ";
        }
        if (regex_match(newLine, regex_left_to_right) || regex_match(newLine, regex_right_to_left)) {
            ret_lines->insert(*it);
        }
    }
    return QueryResult(rep(), ret_lines, result.get_file());
}
/////////////////////////////////////////////////////////