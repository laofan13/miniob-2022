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
// Created by Meiyi 
//

#include <mutex>
#include "sql/parser/parse.h"
#include "rc.h"
#include "common/log/log.h"

RC parse(char *st, Query *sqln);

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus
void relation_attr_init(RelAttr *relation_attr, const char *relation_name, const char *attribute_name)
{
  if (relation_name != nullptr) {
    relation_attr->relation_name = strdup(relation_name);
  } else {
    relation_attr->relation_name = nullptr;
  }
  relation_attr->attribute_name = strdup(attribute_name);
}

void relation_attr_destroy(RelAttr *relation_attr)
{
  free(relation_attr->relation_name);
  free(relation_attr->attribute_name);
  relation_attr->relation_name = nullptr;
  relation_attr->attribute_name = nullptr;
}

void value_init_integer(Value *value, int v)
{
  value->type = INTS;
  value->data = malloc(sizeof(v));
  memcpy(value->data, &v, sizeof(v));
}
void value_init_float(Value *value, float v)
{
  value->type = FLOATS;
  value->data = malloc(sizeof(v));
  memcpy(value->data, &v, sizeof(v));
}
void value_init_string(Value *value, const char *v)
{
  value->type = CHARS;
  value->data = strdup(v);
}
bool check_date(int y, int m, int d) {
  static int mon[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  bool leap = (y%400==0 || (y%100 && y%4==0));
  return y > 0
        && (m > 0) && (m <= 12) 
        && (d > 0)&&(d <= ((m==2 && leap)?1:0) + mon[m]);
}
void value_init_date(Value *value, const char *v) {
  value->type = DATES;
  int y, m, d;
  sscanf(v, "'%d-%d-%d'", &y, &m, &d);
  if(!check_date(y,m,d)) {
    return value_init_string(value, v);
  }
  int dv = y * 10000 + m * 100 + d;
  value->data = malloc(sizeof(dv));
  memcpy(value->data, &dv, sizeof(dv));
}
void value_destroy(Value *value)
{
  value->type = UNDEFINED;
  free(value->data);
  value->data = nullptr;
}

void condition_init(Condition *condition, CompOp comp, int left_is_attr, RelAttr *left_attr, Value *left_value,
    int right_is_attr, RelAttr *right_attr, Value *right_value)
{
  condition->comp = comp;
  condition->left_is_attr = left_is_attr;
  if (left_is_attr) {
    condition->left_attr = *left_attr;
  } else {
    condition->left_value = *left_value;
  }

  condition->right_is_attr = right_is_attr;
  if (right_is_attr) {
    condition->right_attr = *right_attr;
  } else {
    condition->right_value = *right_value;
  }
}
void condition_destroy(Condition *condition)
{
  if (condition->left_is_attr) {
    relation_attr_destroy(&condition->left_attr);
  } else {
    value_destroy(&condition->left_value);
  }
  if (condition->right_is_attr) {
    relation_attr_destroy(&condition->right_attr);
  } else {
    value_destroy(&condition->right_value);
  }
}

void attr_info_init(AttrInfo *attr_info, const char *name, AttrType type, size_t length)
{
  attr_info->name = strdup(name);
  attr_info->type = type;
  if(type == TEXTS) {
    attr_info->length = TEXTSIZE;
  }else{
    attr_info->length = length;
  }

  
}
void attr_info_destroy(AttrInfo *attr_info)
{
  free(attr_info->name);
  attr_info->name = nullptr;
}

void selects_init(Selects *selects, ...);
void selects_append_attribute(Selects *selects, RelAttr *rel_attr)
{
  selects->attributes[selects->attr_num++] = *rel_attr;
}
void selects_append_aggregation(Selects *selects, AggrAttr *aggr_attr)
{
  selects->aggr_attrs[selects->aggr_num++] = *aggr_attr;
}
void selects_append_relation(Selects *selects, const char *relation_name)
{
  selects->relations[selects->relation_num++] = strdup(relation_name);
}

void selects_append_conditions(Selects *selects, Condition conditions[], size_t condition_num)
{
  assert(condition_num <= sizeof(selects->conditions) / sizeof(selects->conditions[0]));
  for (size_t i = 0; i < condition_num; i++) {
    selects->conditions[i] = conditions[i];
  }
  selects->condition_num = condition_num;
}

void selects_append_join_conditions(Selects *selects, Condition conditions[], size_t condition_num)
{
  assert(condition_num <= sizeof(selects->join_conditions) / sizeof(selects->join_conditions[0]));
  for (size_t i = 0; i < condition_num; i++) {
    selects->join_conditions[i] = conditions[i];
  }
  selects->join_num = condition_num;
}
void copy_selects(Selects *selects, Selects *sub_selects) {
   // copy RelAttr
  for(size_t i = 0;i < selects->attr_num; i++) {
    sub_selects->attributes[i] = selects->attributes[i];
  }
  sub_selects->attr_num = selects->attr_num;
  selects->attr_num = 0;

  // copy AggrAttr
  for(size_t i = 0;i < selects->aggr_num; i++) {
    sub_selects->aggr_attrs[i] = selects->aggr_attrs[i];
  }
  sub_selects->aggr_num = selects->aggr_num;
  selects->aggr_num = 0;

  // copy relations
  for(size_t i = 0;i < selects->relation_num; i++) {
    sub_selects->relations[i] = selects->relations[i];
  }
  sub_selects->relation_num = selects->relation_num;
  selects->relation_num = 0;

  // copy join_conditions
  for(size_t i = 0;i < selects->join_num; i++) {
    sub_selects->join_conditions[i] = selects->join_conditions[i];
  }
  sub_selects->join_num = selects->join_num;
  selects->join_num = 0;

  // copy conditions
  for(size_t i = 0;i < selects->condition_num; i++) {
    sub_selects->conditions[i] = selects->conditions[i];
  }
  sub_selects->condition_num = selects->condition_num;
  selects->condition_num = 0;
}


void selects_destroy(Selects *selects)
{
  // destory RelAttr
  for (size_t i = 0; i < selects->attr_num; i++) {
    relation_attr_destroy(&selects->attributes[i]);
  }
  selects->attr_num = 0;

  // destory AggrAttr
  for (size_t i = 0; i < selects->aggr_num; i++) {
    relation_attr_destroy(&selects->aggr_attrs[i].rel_attr);
  }
  selects->aggr_num = 0;

  // destory relations
  for (size_t i = 0; i < selects->relation_num; i++) {
    free(selects->relations[i]);
    selects->relations[i] = NULL;
  }
  selects->relation_num = 0;

  // destory conditions
  for (size_t i = 0; i < selects->condition_num; i++) {
    condition_destroy(&selects->conditions[i]);
  }
  selects->condition_num = 0;

  // destory join_conditions
  for (size_t i = 0; i < selects->join_num; i++) {
    condition_destroy(&selects->join_conditions[i]);
  }
  selects->join_num = 0;
}

void inserts_init(Inserts *inserts, const char *relation_name, size_t record_num)
{
  assert(record_num <= sizeof(inserts->records) / sizeof(inserts->records[0]));
  inserts->relation_name = strdup(relation_name);
  inserts->record_num = record_num;
}
void inserts_record(Inserts *inserts, size_t record_num, Value values[], size_t value_num){
  assert(record_num <= sizeof(inserts->records) / sizeof(inserts->records[0]));

  InsertRecord &insert_record = inserts->records[record_num];

  assert(value_num <= sizeof(insert_record.values) / sizeof(insert_record.values[0]));

  for (size_t i = 0; i < value_num; i++) {
    insert_record.values[i] = values[i];
  }
  insert_record.value_num = value_num;
}


void inserts_destroy(Inserts *inserts)
{
  free(inserts->relation_name);
  inserts->relation_name = nullptr;

  for (size_t i = 0; i < inserts->record_num; i++) {
    InsertRecord &insert_record = inserts->records[i];
    for (size_t i = 0; i < insert_record.value_num; i++) {
      value_destroy(&insert_record.values[i]);
    }
    insert_record.value_num = 0;
  }
  inserts->record_num = 0;
}

void deletes_init_relation(Deletes *deletes, const char *relation_name)
{
  deletes->relation_name = strdup(relation_name);
}

void deletes_set_conditions(Deletes *deletes, Condition conditions[], size_t condition_num)
{
  assert(condition_num <= sizeof(deletes->conditions) / sizeof(deletes->conditions[0]));
  for (size_t i = 0; i < condition_num; i++) {
    deletes->conditions[i] = conditions[i];
  }
  deletes->condition_num = condition_num;
}
void deletes_destroy(Deletes *deletes)
{
  for (size_t i = 0; i < deletes->condition_num; i++) {
    condition_destroy(&deletes->conditions[i]);
  }
  deletes->condition_num = 0;
  free(deletes->relation_name);
  deletes->relation_name = nullptr;
}

void updates_init(Updates *updates, const char *relation_name, Condition conditions[], size_t condition_num)
{
  updates->relation_name = strdup(relation_name);

  assert(condition_num <= sizeof(updates->conditions) / sizeof(updates->conditions[0]));
  for (size_t i = 0; i < condition_num; i++) {
    updates->conditions[i] = conditions[i];
  }
  updates->condition_num = condition_num;
}

void updates_value_append(Updates *updates, const char *attribute_name, Value *value){
  UpdateRecord &update_record = updates->update_records[updates->update_num++];
  update_record.attribute_name = strdup(attribute_name);
  update_record.value = *value;
  update_record.is_sub_select = 0;
}

void updates_select_append(Updates *updates, const char *attribute_name, Selects *selects){
  UpdateRecord &update_record = updates->update_records[updates->update_num++];
  update_record.attribute_name = strdup(attribute_name);

  update_record.is_sub_select = 1;
  copy_selects(selects,&update_record.sub_select);
}

void updates_destroy(Updates *updates)
{
  free(updates->relation_name);
  updates->relation_name = nullptr;

  for(size_t i = 0;i < updates->update_num; i++) {
    UpdateRecord &update_record = updates->update_records[i];
    free(update_record.attribute_name);
    update_record.attribute_name = nullptr;

    if(update_record.is_sub_select) {
      selects_destroy(&update_record.sub_select);
    }else{
      value_destroy(&update_record.value);
    }
  }
  updates->update_num = 0;

  for (size_t i = 0; i < updates->condition_num; i++) {
    condition_destroy(&updates->conditions[i]);
  }
  updates->condition_num = 0;
}

void create_table_append_attribute(CreateTable *create_table, AttrInfo *attr_info)
{
  create_table->attributes[create_table->attribute_count++] = *attr_info;
}

void create_table_init_name(CreateTable *create_table, const char *relation_name)
{
  create_table->relation_name = strdup(relation_name);
}

void create_table_destroy(CreateTable *create_table)
{
  for (size_t i = 0; i < create_table->attribute_count; i++) {
    attr_info_destroy(&create_table->attributes[i]);
  }
  create_table->attribute_count = 0;
  free(create_table->relation_name);
  create_table->relation_name = nullptr;
}

void drop_table_init(DropTable *drop_table, const char *relation_name)
{
  drop_table->relation_name = strdup(relation_name);
}

void drop_table_destroy(DropTable *drop_table)
{
  free(drop_table->relation_name);
  drop_table->relation_name = nullptr;
}

void create_index_init(CreateIndex *create_index, const char *index_name, const char *relation_name)
{
  create_index->index_name = strdup(index_name);
  create_index->relation_name = strdup(relation_name);
}

void create_index_append(CreateIndex *create_index, const char *attr_name) {
  create_index->attributes[create_index->attr_num++] = strdup(attr_name);
}

void create_index_destroy(CreateIndex *create_index)
{
  free(create_index->index_name);
  free(create_index->relation_name);

  create_index->index_name = nullptr;
  create_index->relation_name = nullptr;

  for(int i = 0;i < create_index->attr_num;i++) {
    free(create_index->attributes[i]);
    create_index->attributes[i] = nullptr;
  }
}

void show_index_init(ShowIndex *show_index, const char *relation_name)
{
  show_index->relation_name = strdup(relation_name);
}

void show_index_destroy(ShowIndex *show_index)
{
  free((char *)show_index->relation_name);
  show_index->relation_name = nullptr;
}

void drop_index_init(DropIndex *drop_index, const char *index_name)
{
  drop_index->index_name = strdup(index_name);
}

void drop_index_destroy(DropIndex *drop_index)
{
  free((char *)drop_index->index_name);
  drop_index->index_name = nullptr;
}

void desc_table_init(DescTable *desc_table, const char *relation_name)
{
  desc_table->relation_name = strdup(relation_name);
}

void desc_table_destroy(DescTable *desc_table)
{
  free((char *)desc_table->relation_name);
  desc_table->relation_name = nullptr;
}

void load_data_init(LoadData *load_data, const char *relation_name, const char *file_name)
{
  load_data->relation_name = strdup(relation_name);

  if (file_name[0] == '\'' || file_name[0] == '\"') {
    file_name++;
  }
  char *dup_file_name = strdup(file_name);
  int len = strlen(dup_file_name);
  if (dup_file_name[len - 1] == '\'' || dup_file_name[len - 1] == '\"') {
    dup_file_name[len - 1] = 0;
  }
  load_data->file_name = dup_file_name;
}

void load_data_destroy(LoadData *load_data)
{
  free((char *)load_data->relation_name);
  free((char *)load_data->file_name);
  load_data->relation_name = nullptr;
  load_data->file_name = nullptr;
}

void query_init(Query *query)
{
  query->flag = SCF_ERROR;
  memset(&query->sstr, 0, sizeof(query->sstr));
}

Query *query_create()
{
  Query *query = (Query *)malloc(sizeof(Query));
  if (nullptr == query) {
    LOG_ERROR("Failed to alloc memroy for query. size=%ld", sizeof(Query));
    return nullptr;
  }

  query_init(query);
  return query;
}

void query_reset(Query *query)
{
  switch (query->flag) {
    case SCF_SELECT: {
      selects_destroy(&query->sstr.selection);
    } break;
    case SCF_INSERT: {
      inserts_destroy(&query->sstr.insertion);
    } break;
    case SCF_DELETE: {
      deletes_destroy(&query->sstr.deletion);
    } break;
    case SCF_UPDATE: {
      updates_destroy(&query->sstr.update);
    } break;
    case SCF_CREATE_TABLE: {
      create_table_destroy(&query->sstr.create_table);
    } break;
    case SCF_DROP_TABLE: {
      drop_table_destroy(&query->sstr.drop_table);
    } break;
    case SCF_CREATE_INDEX: {
      create_index_destroy(&query->sstr.create_index);
    } break;
    case SCF_SHOW_INDEX: {
      show_index_destroy(&query->sstr.show_index);
    } break;
    case SCF_DROP_INDEX: {
      drop_index_destroy(&query->sstr.drop_index);
    } break;
    case SCF_SYNC: {

    } break;
    case SCF_SHOW_TABLES:
      break;

    case SCF_DESC_TABLE: {
      desc_table_destroy(&query->sstr.desc_table);
    } break;

    case SCF_LOAD_DATA: {
      load_data_destroy(&query->sstr.load_data);
    } break;
    case SCF_CLOG_SYNC:
    case SCF_BEGIN:
    case SCF_COMMIT:
    case SCF_ROLLBACK:
    case SCF_HELP:
    case SCF_EXIT:
    case SCF_ERROR:
      break;
  }
}

void query_destroy(Query *query)
{
  query_reset(query);
  free(query);
}
#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

////////////////////////////////////////////////////////////////////////////////

extern "C" int sql_parse(const char *st, Query *sqls);

RC parse(const char *st, Query *sqln)
{
  sql_parse(st, sqln);

  if (sqln->flag == SCF_ERROR)
    return SQL_SYNTAX;
  else
    return SUCCESS;
}