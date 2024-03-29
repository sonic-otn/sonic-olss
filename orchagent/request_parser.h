#ifndef __REQUEST_PARSER_H
#define __REQUEST_PARSER_H

#include "ipaddress.h"
#include "ipprefix.h"
#include <sstream>
#include <set>

typedef enum _request_types_t
{
    REQ_T_NOT_USED,
    REQ_T_BOOL,
    REQ_T_STRING,
    REQ_T_MAC_ADDRESS,
    REQ_T_PACKET_ACTION,
    REQ_T_IP,
    REQ_T_IP_PREFIX,
    REQ_T_VLAN,
    REQ_T_UINT,
    REQ_T_SET,
} request_types_t;

typedef struct _request_description
{
    std::vector<request_types_t> key_item_types;
    std::unordered_map<std::string, request_types_t> attr_item_types;
    std::vector<std::string> mandatory_attr_items;
} request_description_t;

class Request
{
public:
    void parse(const swss::KeyOpFieldsValuesTuple& request);
    void clear();

    const std::string& getOperation() const
    {
        assert(is_parsed_);
        return operation_;
    }

    const std::string& getFullKey() const
    {
        assert(is_parsed_);
        return full_key_;
    }

    const std::string& getKeyString(int position) const
    {
        assert(is_parsed_);
        return key_item_strings_.at(position);
    }

    const uint64_t& getKeyUint(int position) const
    {
        assert(is_parsed_);
        return key_item_uint_.at(position);
    }

    const std::unordered_set<std::string>& getAttrFieldNames() const
    {
        assert(is_parsed_);
        return attr_names_;
    }

    const std::string& getAttrString(const std::string& attr_name) const
    {
        assert(is_parsed_);
        return attr_item_strings_.at(attr_name);
    }

    bool getAttrBool(const std::string& attr_name) const
    {
        assert(is_parsed_);
        return attr_item_bools_.at(attr_name);
    }

    const uint64_t& getAttrUint(const std::string& attr_name) const
    {
        assert(is_parsed_);
        return attr_item_uint_.at(attr_name);
    }

    const std::set<std::string>& getAttrSet(const std::string& attr_name) const
    {
        assert(is_parsed_);
        return attr_item_set_.at(attr_name);
    }

    void setTableName(std::string& table_name)
    {
        table_name_ = table_name;
    }

    const std::string& getTableName() const
    {
        assert(is_parsed_);
        return table_name_;
    }

protected:
    Request(const request_description_t& request_description, const char key_separator)
        : request_description_(request_description),
          key_separator_(key_separator),
          is_parsed_(false),
          number_of_key_items_(request_description.key_item_types.size())
    {
    }


private:
    void parseOperation(const swss::KeyOpFieldsValuesTuple& request);
    void parseKey(const swss::KeyOpFieldsValuesTuple& request);
    void parseAttrs(const swss::KeyOpFieldsValuesTuple& request);
    bool parseBool(const std::string& str);
    uint64_t parseUint(const std::string& str);
    uint16_t parseVlan(const std::string& str);
    std::set<std::string> parseSet(const std::string& str);

    const request_description_t& request_description_;
    char key_separator_;
    bool is_parsed_;
    size_t number_of_key_items_;

    std::string table_name_;
    std::string operation_;
    std::string full_key_;
    std::unordered_map<int, std::string> key_item_strings_;
    std::unordered_map<int, uint64_t> key_item_uint_;
    std::unordered_set<std::string> attr_names_;
    // FIXME: Make one union with all the values, except string
    std::unordered_map<std::string, std::string> attr_item_strings_;
    std::unordered_map<std::string, bool> attr_item_bools_;
    std::unordered_map<std::string, uint16_t> attr_item_vlan_;
    std::unordered_map<std::string, uint64_t> attr_item_uint_;
    std::unordered_map<std::string, std::set<std::string>> attr_item_set_;
};

#endif // __REQUEST_PARSER_H
