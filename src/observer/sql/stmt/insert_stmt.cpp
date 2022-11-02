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
// Created by Wangyunlai on 2022/5/22.
//
#include<cmath>
#include <string>
#include<sstream>

#include "sql/stmt/insert_stmt.h"
#include "common/log/log.h"
#include "storage/common/db.h"
#include "storage/common/table.h"
#include "util/util.h"


InsertStmt::InsertStmt(Table *table, const InsertRecord *records, int record_amount)
  : table_ (table), records_(records), record_amount_(record_amount)
{}

RC InsertStmt::create(Db *db, Inserts &inserts, Stmt *&stmt)
{
  const char *table_name = inserts.relation_name;
  if (nullptr == db || nullptr == table_name || inserts.record_num <= 0) {
    LOG_WARN("invalid argument. db=%p, table_name=%p, record_num=%d", 
             db, table_name, inserts.record_num);
    return RC::INVALID_ARGUMENT;
  }

  // check whether the table exists
  Table *table = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  // check the fields number
  for(int i = 0; i < inserts.record_num; i++) {
    Value *values = inserts.records[i].values;
    const int value_num = inserts.records[i].value_num;
    const TableMeta &table_meta = table->table_meta();
    const int field_num = table_meta.field_num() - table_meta.sys_field_num();
    if (field_num != value_num) {
      LOG_WARN("schema mismatch. value num=%d, field num in schema=%d", value_num, field_num);
      return RC::SCHEMA_FIELD_MISSING;
    }

    // check fields type
    const int sys_field_num = table_meta.sys_field_num();
    for (int i = 0; i < value_num; i++) {
      const FieldMeta *field_meta = table_meta.field(i + sys_field_num);
      const AttrType field_type = field_meta->type();
      const AttrType value_type = values[i].type;
      void *data = values[i].data;

      if(value_type == NULLS && !field_meta->nullable()) { // 值是空类型
        LOG_WARN("field type mismatch. table=%s, field=%s, field is not nullable, value_type=%d", 
                table_name, field_meta->name(),  value_type);
        return RC::SCHEMA_FIELD_TYPE_MISMATCH;
      }else{
        continue;
      }

      if (field_type != value_type) { // TODO try to convert the value type to field type
        if (field_type == INTS && value_type == FLOATS) {
          int val = round(*(float *)data);
          *(int *)data = val;
        }else if (field_type == INTS && value_type == CHARS) {
          int val = std::atoi((char *)(data));
          *(int *)data = val;
        }else if (field_type == FLOATS && value_type == INTS) {
          float val = *(int *)data;
          *(float *)data = val;
        }else if (field_type == FLOATS && value_type == CHARS) {
          float val = std::atof((char *)(data));
          *(float *)data = val;
        }else if (field_type == CHARS && value_type == INTS) {
         std::string s = std::to_string(*(int *)data);
         char *str = (char *)(data);
         int i =0;
         for(;i < 4 && i < s.size();i++) {
          str[i] = s[i];
         }
         if(i < 4) {
           str[i] = '\0';
         }else{
           str[4] = '\0';
         }

        }else if (field_type == CHARS && value_type == FLOATS) {
          std::ostringstream oss;
          oss<<*(float *)data;
          std::string s(oss.str());
          char *str = (char *)(data);
          int i =0;
          for(;i < 4 && i < s.size();i++) {
            str[i] = s[i];
          }
          if(i < 4) {
            str[i] = '\0';
          }else{
            str[4] = '\0';
          }
        }else if(field_type == TEXTS && value_type == CHARS){
          values[i].type = TEXTS;
        }else{
          LOG_WARN("field type mismatch. table=%s, field=%s, field type=%d, value_type=%d", 
                table_name, field_meta->name(), field_type, value_type);
          return RC::SCHEMA_FIELD_TYPE_MISMATCH;
        }
      }
    }
  }

  // everything alright
  stmt = new InsertStmt(table, inserts.records, inserts.record_num);
  return RC::SUCCESS;
}
