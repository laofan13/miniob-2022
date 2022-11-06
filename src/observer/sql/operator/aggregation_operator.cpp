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
// Created by WangYunlai on 2021/6/9.
//

#pragma once

#include <memory>
#include <unordered_map>
#include <utility>
#include <functional>

#include "sql/operator/aggregation_operator.h"
#include "sql/stmt/select_stmt.h"
#include "storage/common/table.h"
#include "rc.h"

RC AggregationOperator::open()
{
  if (children_.size() != 1) {
    LOG_WARN("delete operator must has 1 child");
    return RC::INTERNAL;
  }

  Operator *child = children_[0];
  RC rc = child->open();
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  while (RC::SUCCESS == (rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("failed to get current record: %s", strrc(rc));
      return rc;
    }
    aht_.InsertCombine(MakeAggregateKey(tuple), MakeAggregateValue(tuple));
  }
  aht_iterator_ = aht_.Begin();

  return RC::SUCCESS;
}

RC AggregationOperator::next()
{
  if (aht_iterator_ == aht_.End()) {
    return RC::RECORD_EOF;
  }
  AggregateKey key = aht_iterator_.Key();
  AggregateValue val = aht_iterator_.Val();

  tuple_.set_group_key(key.group_bys_);
  tuple_.set_aggregate_value(val.aggregates_);
  tuple_.set_aggregate_num(val.aggregates_num_);

  ++aht_iterator_;
  return RC::SUCCESS;
}

RC AggregationOperator::close()
{
  children_[0]->close();
  return RC::SUCCESS;
}

void AggregationOperator::add_projection(const Table *table, const FieldMeta *field_meta)
{
  // 对单表来说，展示的(alias) 字段总是字段名称，
  // 对多表查询来说，展示的alias 需要带表名字
  TupleCellSpec *spec = new TupleCellSpec(new FieldExpr(table, field_meta));
  spec->set_alias(field_meta->name());
  tuple_.add_cell_spec(spec);
}

void AggregationOperator::add_projection(const Table *table, const FieldMeta *field_meta, AggrType aggr_type) {
  TupleCellSpec *spec = new TupleCellSpec(new AggrExpr(table, field_meta,aggr_type));
  spec->set_alias(field_meta->name());
  tuple_.add_cell_spec(spec);
}

Tuple * AggregationOperator::current_tuple(){
  return &tuple_;
}