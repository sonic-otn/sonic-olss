#include <net/ethernet.h>
#include <cassert>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <exception>

#include "otai.h"
#include "orch.h"
#include "request_parser.h"

using namespace std;
using namespace swss;


void Request::parse(const KeyOpFieldsValuesTuple& request)
{
    if (is_parsed_)
    {
        throw std::logic_error("The parser already has a parsed request");
    }

    parseOperation(request);
    parseKey(request);
    parseAttrs(request);

    is_parsed_ = true;
}

void Request::clear()
{
    operation_.clear();
    full_key_.clear();
    attr_names_.clear();
    key_item_strings_.clear();
    attr_item_strings_.clear();
    attr_item_bools_.clear();

    is_parsed_ = false;
}

void Request::parseOperation(const KeyOpFieldsValuesTuple& request)
{
    operation_ = kfvOp(request);
    if (operation_ != SET_COMMAND && operation_ != DEL_COMMAND)
    {
        throw std::invalid_argument(std::string("Wrong operation: ") + operation_);
    }
}

void Request::parseKey(const KeyOpFieldsValuesTuple& request)
{
    full_key_ = kfvKey(request);

    // split the key by separator
    std::vector<std::string> key_items;
    size_t key_item_start = 0;
    size_t key_item_end = full_key_.find(key_separator_);
    while (key_item_end != std::string::npos)
    {
        key_items.push_back(full_key_.substr(key_item_start, key_item_end - key_item_start));
        key_item_start = key_item_end + 1;
        key_item_end = full_key_.find(key_separator_, key_item_start);
    }
    key_items.push_back(full_key_.substr(key_item_start, full_key_.length()));

    if (key_items.size() != number_of_key_items_)
    {
        throw std::invalid_argument(std::string("Wrong number of key items. Expected ")
                                  + std::to_string(number_of_key_items_)
                                  + std::string(" item(s). Key: '")
                                  + full_key_
                                  + std::string("'"));
    }

    // check types of the key items
    for (int i = 0; i < static_cast<int>(number_of_key_items_); i++)
    {
        switch (request_description_.key_item_types[i])
        {
            case REQ_T_STRING:
                key_item_strings_[i] = key_items[i];
                break;
            case REQ_T_UINT:
                key_item_uint_[i] = parseUint(key_items[i]);
                break;
            default:
                throw std::logic_error(std::string("Not implemented key type parser. Key '")
                                     + full_key_
                                     + std::string("'. Key item:")
                                     + key_items[i]);
        }
    }
}

void Request::parseAttrs(const KeyOpFieldsValuesTuple& request)
{
    const auto not_found = std::end(request_description_.attr_item_types);

    for (auto i = kfvFieldsValues(request).begin();
         i != kfvFieldsValues(request).end(); i++)
    {
        if (fvField(*i) == "empty" || fvField(*i) == "NULL")
        {
            // if name of the attribute is 'empty' or 'NULL', just skip it.
            // it's used when we don't have any attributes, but we have to provide one for redis
            continue;
        }
        const auto item = request_description_.attr_item_types.find(fvField(*i));
        if (item == not_found)
        {
            throw std::invalid_argument(std::string("Unknown attribute name: ") + fvField(*i));
        }
        attr_names_.insert(fvField(*i));
        switch (item->second)
        {
            case REQ_T_STRING:
                attr_item_strings_[fvField(*i)] = fvValue(*i);
                break;
            case REQ_T_BOOL:
                attr_item_bools_[fvField(*i)] = parseBool(fvValue(*i));
                break;
            case REQ_T_UINT:
                attr_item_uint_[fvField(*i)] = parseUint(fvValue(*i));
                break;
            case REQ_T_SET:
                attr_item_set_[fvField(*i)] = parseSet(fvValue(*i));
                break;
            default:
                throw std::logic_error(std::string("Not implemented attribute type parser for attribute:") + fvField(*i));
        }
    }

    if (operation_ == DEL_COMMAND && attr_names_.size() > 0)
    {
        throw std::invalid_argument("Delete operation request contains attributes");
    }

    if (operation_ == SET_COMMAND)
    {
        for (const auto& attr: request_description_.mandatory_attr_items)
        {
            if (attr_names_.find(attr) == std::end(attr_names_))
            {
                throw std::invalid_argument(std::string("Mandatory attribute '") + attr + std::string("' not found"));
            }
        }
    }
}

bool Request::parseBool(const std::string& str)
{
    if (str == "true")
    {
        return true;
    }

    if (str == "false")
    {
        return false;
    }

    throw std::invalid_argument(std::string("Can't parse boolean value '") + str + std::string("'"));
}

set<string> Request::parseSet(const std::string& str)
{
    try
    {
        set<string> str_set;
        string substr;
        std::istringstream iss(str);
        while (getline(iss, substr, ','))
        {
            str_set.insert(substr);
        }
        return str_set;
    }
    catch (std::invalid_argument& _)
    {
        throw std::invalid_argument(std::string("Invalid string set"));
    }
}

uint64_t Request::parseUint(const std::string& str)
{
    try
    {
        uint64_t ret = std::stoul(str);
        return ret;
    }
    catch(std::invalid_argument& _)
    {
        throw std::invalid_argument(std::string("Invalid unsigned integer: ") + str);
    }
    catch(std::out_of_range& _)
    {
        throw std::invalid_argument(std::string("Out of range unsigned integer: ") + str);
    }
}

