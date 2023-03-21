/**
 * Copyright (c) 2023 Alibaba Group Holding Limited
 *
 *    Licensed under the Apache License, Version 2.0 (the "License"); you may
 *    not use this file except in compliance with the License. You may obtain
 *    a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 *    THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
 *    CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 *    LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 *    FOR A PARTICULAR PURPOSE, MERCHANTABILITY OR NON-INFRINGEMENT.
 *
 *    See the Apache Version 2.0 License for specific language governing
 *    permissions and limitations under the License.
 *
 */

#include "lldporch.h"
#include "flexcounterorch.h"

using namespace std;
using namespace swss;

extern FlexCounterOrch *gFlexCounterOrch;

extern std::unordered_set<std::string> lldp_counter_ids_status;

vector<lai_attr_id_t> g_lldp_cfg_attrs =
{
    LAI_LLDP_ATTR_CHANNEL_ID,
    LAI_LLDP_ATTR_ENABLED,
};

vector<string> g_lldp_auxiliary_fields =
{
    "index",
};

LldpOrch::LldpOrch(DBConnector *db, const vector<string> &table_names)
    : LaiObjectOrch(db, table_names, LAI_OBJECT_TYPE_LLDP, g_lldp_cfg_attrs, g_lldp_auxiliary_fields)
{
    m_stateTable = unique_ptr<Table>(new Table(m_stateDb.get(), STATE_LLDP_TABLE_NAME));
    m_countersTable = COUNTERS_LLDP_TABLE_NAME;
    m_nameMapTable = unique_ptr<Table>(new Table(m_countersDb.get(), COUNTERS_LLDP_NAME_MAP));

    m_notificationConsumer = new NotificationConsumer(db, LLDP_NOTIFICATION);
    auto notifier = new Notifier(m_notificationConsumer, this, LLDP_NOTIFICATION);
    Orch::addExecutor(notifier);
    m_notificationProducer = new NotificationProducer(db, LLDP_REPLY);

    m_createFunc = lai_lldp_api->create_lldp;
    m_removeFunc = lai_lldp_api->remove_lldp;
    m_setFunc = lai_lldp_api->set_lldp_attribute;
    m_getFunc = lai_lldp_api->get_lldp_attribute;
}

void LldpOrch::setFlexCounter(lai_object_id_t id, vector<lai_attribute_t> &attrs)
{            
    gFlexCounterOrch->getStatusGroup()->setCounterIdList(id, CounterType::LLDP_STATUS, lldp_counter_ids_status);
}

