/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022/6/6.
//

#include "sql/stmt/select_stmt.h"
#include "sql/stmt/filter_stmt.h"
#include "sql/stmt/join_stmt.h"

#include "common/log/log.h"
#include "common/lang/string.h"
#include "storage/common/db.h"
#include "storage/common/table.h"

#include "storage/common/table.h"

SelectStmt::~SelectStmt()
{
  if (nullptr != filter_stmt_) {
    delete filter_stmt_;
    filter_stmt_ = nullptr;
  }
}

static void wildcard_fields(Table *table, std::vector<Field> &field_metas)
{
  const TableMeta &table_meta = table->table_meta();
  const int field_num = table_meta.field_num();
  for (int i = table_meta.sys_field_num(); i < field_num; i++) {
    field_metas.push_back(Field(table, table_meta.field(i)));
  }
}

static RC get_tables(Db *db,size_t relation_num, char *const *relations,
                    std::vector<Table *> &tables,
                    std::unordered_map<std::string, Table *> &table_map) {
  for (int i = relation_num-1; i >= 0; i--) {
    const char *table_name = relations[i];
    if (nullptr == table_name) {
      LOG_WARN("invalid argument. relation name is null. index=%d", i);
      return RC::INVALID_ARGUMENT;
    }

    Table *table = db->find_table(table_name);
    if (nullptr == table) {
      LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
      return RC::SCHEMA_TABLE_NOT_EXIST;
    }

    tables.push_back(table);
    table_map.insert(std::pair<std::string, Table*>(table_name, table));
  }
  return RC::SUCCESS;
}

static RC get_query_fields(Db *db,std::vector<Table *> &tables, 
                            std::unordered_map<std::string, Table *> &table_map, 
                            size_t attr_num, const RelAttr *attributes,
                            std::vector<Field> &query_fields){
  for (size_t i = 0; i < attr_num; i++) {
    const RelAttr &relation_attr = attributes[i];
    const char *table_name = relation_attr.relation_name;
    const char *field_name = relation_attr.attribute_name;

    if (common::is_blank(table_name)) {
      if(0 == strcmp(field_name, "*")) {
        for (Table *table : tables) {
          wildcard_fields(table, query_fields);
        }
      }else{
        if (tables.size() != 1) {
          LOG_WARN("invalid. I do not know the attr's table. attr=%s", field_name);
          return RC::SCHEMA_FIELD_MISSING;
        }

        Table *table = tables[0];
        const FieldMeta *field_meta = table->table_meta().field(field_name);
        if (nullptr == field_meta) {
          LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), field_name);
          return RC::SCHEMA_FIELD_MISSING;
        }
        query_fields.push_back(Field(table, field_meta));
      }
    } else { // TODO
      if (0 == strcmp(table_name, "*")) {
        if (0 != strcmp(field_name, "*")) {
          LOG_WARN("invalid field name while table is *. attr=%s", field_name);
          return RC::SCHEMA_FIELD_MISSING;
        }
        for (Table *table : tables) {
          wildcard_fields(table, query_fields);
        }
      } else {
        auto iter = table_map.find(table_name);
        if (iter == table_map.end()) {
          LOG_WARN("no such table in from list: %s", table_name);
          return RC::SCHEMA_FIELD_MISSING;
        }

        Table *table = iter->second;
        if (0 == strcmp(field_name, "*")) {
          wildcard_fields(table, query_fields);
        } else {
          const FieldMeta *field_meta = table->table_meta().field(field_name);
          if (nullptr == field_meta) {
            LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), field_name);
            return RC::SCHEMA_FIELD_MISSING;
          }
          query_fields.push_back(Field(table, field_meta));
        }
      }
    } 
  }

  LOG_INFO("got %d tables in from stmt and %d fields in query stmt", tables.size(), query_fields.size());
  return RC::SUCCESS;                          
}


// 获取聚合字段
static RC get_aggr_fields( Db *db,std::vector<Table *> &tables, 
                           std::unordered_map<std::string, Table *> &table_map, 
                           size_t aggr_num, const AggrAttr *aggr_attrs,
                           std::vector<AggrField> &aggr_fields){
  assert(tables.size() > 0);

  for (size_t i = 0; i < aggr_num; i++) {
    const char *table_name = aggr_attrs[i].rel_attr.relation_name;
    const char *field_name = aggr_attrs[i].rel_attr.attribute_name;
    AggrType aggr_type = aggr_attrs[i].aggr_type;

    if (common::is_blank(table_name)) {
      if(0 == strcmp(field_name, "*")) {
        if(aggr_type != AggrType::COUNT_FUNC) {
          LOG_WARN("only aggregation function is *. attr=%s", field_name);
          return RC::SCHEMA_FIELD_MISSING;
        }
        const Table *table = tables[0];
        const FieldMeta *field_meta = table->table_meta().field(0);
        AggrField aggr_field(table, field_meta, aggr_type);
        aggr_field.set_aggr_name(field_name);
        aggr_fields.push_back(aggr_field);
      }else{
        if (tables.size() != 1) {
          LOG_WARN("invalid. I do not know the attr's table. attr=%s", field_name);
          return RC::SCHEMA_FIELD_MISSING;
        }

        const Table *table = tables[0];
        const FieldMeta *field_meta = table->table_meta().field(field_name);
        if (nullptr == field_meta) {
          LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), field_name);
          return RC::SCHEMA_FIELD_MISSING;
        }
        AggrField aggr_field(table, field_meta, aggr_type);
        aggr_field.set_aggr_name(field_name);
        aggr_fields.push_back(aggr_field);
      }
    }else { // TODO
      if (0 == strcmp(table_name, "*")) {
        if (0 != strcmp(field_name, "*")) {
          LOG_WARN("invalid field name while table is *. attr=%s", field_name);
          return RC::SCHEMA_FIELD_MISSING;
        }
        if(aggr_type != AggrType::COUNT_FUNC) {
          LOG_WARN("only aggregation function is *. attr=%s", field_name);
          return RC::SCHEMA_FIELD_MISSING;
        }
        const Table *table = tables[0];
        const FieldMeta *field_meta = table->table_meta().field(field_name);
        if (nullptr == field_meta) {
          LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), field_name);
          return RC::SCHEMA_FIELD_MISSING;
        }
        AggrField aggr_field(table, field_meta, aggr_type);
        aggr_field.set_aggr_name(field_name);
        aggr_fields.push_back(aggr_field);
      } else {
        auto iter = table_map.find(table_name);
        if (iter == table_map.end()) {
          LOG_WARN("no such table in from list: %s", table_name);
          return RC::SCHEMA_FIELD_MISSING;
        }

        Table *table = iter->second;
        const FieldMeta *field_meta = table->table_meta().field(field_name);
        if (nullptr == field_meta) {
          LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), field_name);
          return RC::SCHEMA_FIELD_MISSING;
        }
        AggrField aggr_field(table, field_meta, aggr_type);
        aggr_field.set_aggr_name(field_name);
        aggr_fields.push_back(aggr_field);
      }
    } 
  }

  return RC::SUCCESS;                          
}

// 获取排序字段
static RC get_sort_fields(Db *db, Table *default_table, std::unordered_map<std::string, Table *> &table_map, 
                           size_t order_num, const OrderAttr *order_attributes,
                           std::vector<SortField> &sort_fields){
  RC rc = RC::SUCCESS;
  // 默认字段
  for (int i = 0; i < order_num; i++) {
    const OrderAttr &order_attr = order_attributes[i];

    Table *table = nullptr;
    const FieldMeta *field = nullptr;
    rc = FilterStmt::get_table_and_field(db, default_table, &table_map, order_attr.rel_attr, table, field);  
    if (rc != RC::SUCCESS) {
      LOG_WARN("cannot find attr");
      return rc;
    }
    sort_fields.push_back(SortField(table, field, order_attr.order_type));
  }

  return rc;                          
}

// 获取聚合字段
static RC get_group_fields(Db *db, Table *default_table, std::unordered_map<std::string, Table *> &table_map, 
                           size_t group_num, const RelAttr *attributes,
                           std::vector<Field> &group_fields){
  RC rc = RC::SUCCESS;
  // 默认字段
  for (int i = 0; i < group_num; i++) {
    const RelAttr &rel_attr = attributes[i];

    Table *table = nullptr;
    const FieldMeta *field = nullptr;
    rc = FilterStmt::get_table_and_field(db, default_table, &table_map, rel_attr, table, field);  
    if (rc != RC::SUCCESS) {
      LOG_WARN("cannot find attr");
      return rc;
    }
    group_fields.push_back(Field(table, field));
  }

  return rc;                          
}

RC SelectStmt::create(Db *db, const Selects &select_sql, Stmt *&stmt)
{
  RC rc = RC::SUCCESS;
  if (nullptr == db) {
    LOG_WARN("invalid argument. db is null");
    return RC::INVALID_ARGUMENT;
  }
  
  // collect tables in `from` statement
  std::vector<Table *> tables;
  std::unordered_map<std::string, Table *> table_map;
  rc = get_tables(db,select_sql.relation_num,select_sql.relations,tables,table_map);
  if(rc != RC::SUCCESS) {
    LOG_WARN("Failed to get table from sql statement");
    return rc;
  }

  if(tables.size() < 1) {
    LOG_WARN("invalid. I do not know the attr's table.");
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }
  
  // collect query fields in `select` statement
  std::vector<Field> query_fields;
  rc = get_query_fields(db,tables, table_map, select_sql.attr_num, select_sql.attributes, query_fields);
  if(rc != RC::SUCCESS) {
    return rc;
  }

  // collect aggr fields in `select` statement
  std::vector<AggrField> aggr_fields;
  rc = get_aggr_fields(db,tables, table_map, select_sql.aggr_num, select_sql.aggr_attributes, aggr_fields);
  if(rc != RC::SUCCESS) {
    return rc;
  }
  

  Table *default_table = nullptr;
  if (tables.size() == 1) {
    default_table = tables[0];
  }

  // create filter statement in `where` statement
  FilterStmt *filter_stmt = nullptr;
  rc = FilterStmt::create(db, default_table, &table_map,
          select_sql.conditions, select_sql.condition_num, filter_stmt);
  if (rc != RC::SUCCESS) {
    LOG_WARN("cannot construct filter stmt");
    return rc;
  }

   // create join statement 
  JoinStmt *join_stmt = nullptr;
  rc = JoinStmt::create(db, tables, table_map,
          select_sql.join_conditions, select_sql.join_num, join_stmt);
  if (rc != RC::SUCCESS) {
    LOG_WARN("cannot construct join stmt");
    return rc;
  }
  
  // order by statement in `order by` statement
  std::vector<SortField> sort_fields;
  rc = get_sort_fields(db, default_table,table_map, select_sql.order_num, select_sql.order_attributes, sort_fields);
  if(rc != RC::SUCCESS) {
    return rc;
  }

  // order by statement in `group by` statement
  std::vector<Field> group_fields;
  rc = get_group_fields(db, default_table,table_map, select_sql.group_num, select_sql.group_attrs, group_fields);
  if(rc != RC::SUCCESS) {
    return rc;
  }

  // everything alright
  SelectStmt *select_stmt = new SelectStmt();
  select_stmt->tables_.swap(tables);
  select_stmt->query_fields_.swap(query_fields);
  select_stmt->aggr_fields_.swap(aggr_fields);

  select_stmt->filter_stmt_ = filter_stmt;
  select_stmt->join_stmt_ = join_stmt;

  select_stmt->sort_fields_.swap(sort_fields);
  select_stmt->group_fields_.swap(group_fields);
  
  stmt = select_stmt;
  return RC::SUCCESS;
}

bool SelectStmt::is_inner_join() {
  if(join_stmt_) {
    return !join_stmt_->join_units().empty();
  }
  return false;
}