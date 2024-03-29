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
// Created by Wangyunlai on 2021/5/14.
//

#pragma once

#include <memory>
#include <vector>

#include "common/log/log.h"
#include "sql/parser/parse.h"
#include "sql/expr/tuple_cell.h"
#include "sql/expr/expression.h"
#include "storage/record/record.h"

class Table;

class TupleCellSpec
{
public:
  TupleCellSpec() = default;
  TupleCellSpec(Expression *expr) : expression_(expr)
  {}

  ~TupleCellSpec()
  {
    if (expression_) {
      delete expression_;
      expression_ = nullptr;
    }
  }

  void set_alias(const char *alias)
  {
    this->alias_ = alias;
  }
  const char *alias() const
  {
    return alias_;
  }

  Expression *expression() const
  {
    return expression_;
  }

private:
  const char *alias_ = nullptr;
  Expression *expression_ = nullptr;
};

class Tuple
{
public:
  Tuple() = default;
  virtual ~Tuple() = default;

  virtual int cell_num() const = 0; 
  virtual RC  cell_at(int index, TupleCell &cell) const = 0;
  virtual RC  find_cell(const Field &field, TupleCell &cell) const = 0;

  virtual RC  cell_spec_at(int index, const TupleCellSpec *&spec) const = 0;
};

class RowTuple : public Tuple
{
public:
  RowTuple() = default;
  RowTuple(const Table *table, const std::vector<FieldMeta> *fields){
    table_ = table;
    null_meta_ = table_->table_meta().null_meta();
    this->speces_.reserve(fields->size());
    for (const FieldMeta &field : *fields) {
      speces_.push_back(std::make_shared<TupleCellSpec>(new FieldExpr(table, &field)));
    }
  }
  virtual ~RowTuple()
  {
    // for (TupleCellSpec *spec : speces_) {
    //   delete spec;
    // }
    // speces_.clear();
  }
  
  void set_record(Record *record)
  {
    this->record_ = record;

    // 首先获取 null_meta
    memcpy(&null_map_,record->data() + null_meta_->offset(),null_meta_->len());
  }

  void set_schema(const Table *table, const std::vector<FieldMeta> *fields)
  {
    table_ = table;
    null_meta_ = table_->table_meta().null_meta();
    this->speces_.reserve(fields->size());
    for (const FieldMeta &field : *fields) {
      speces_.push_back(std::make_shared<TupleCellSpec>(new FieldExpr(table, &field)));
    }
  }

  int cell_num() const override
  {
    return speces_.size();
  }

  RC cell_at(int index, TupleCell &cell) const override
  {
    if (index < 0 || index >= static_cast<int>(speces_.size())) {
      LOG_WARN("invalid argument. index=%d", index);
      return RC::INVALID_ARGUMENT;
    }

    const TupleCellSpec *spec = speces_[index].get();;
    FieldExpr *field_expr = (FieldExpr *)spec->expression();
    const FieldMeta *field_meta = field_expr->field().meta();

    char *data = this->record_->data();
    if(null_map_ & (1 << index)) {
      cell.set_type(NULLS);
    }else{
      cell.set_type(field_meta->type());
      cell.set_data(data+ field_meta->offset());
      cell.set_length(field_meta->len());

      if(field_meta->type() == TEXTS) {
        cell.new_text();
        RC rc = table_->read_text_record(data + field_meta->offset(), cell.text_data());
        if(rc != RC::SUCCESS) {
          return rc;
        }
      }
    }
   
    return RC::SUCCESS;
  }

  RC find_cell(const Field &field, TupleCell &cell) const override
  {
    const char *table_name = field.table_name();
    if (0 != strcmp(table_name, table_->name())) {
      return RC::NOTFOUND;
    }

    const char *field_name = field.field_name();
    for (size_t i = 0; i < speces_.size(); ++i) {
      const FieldExpr * field_expr = (const FieldExpr *)speces_[i]->expression();
      const Field &field = field_expr->field();
      if (0 == strcmp(field_name, field.field_name())) {
	      return cell_at(i, cell);
      }
    }
    return RC::NOTFOUND;
  }

  RC cell_spec_at(int index, const TupleCellSpec *&spec) const override
  {
    if (index < 0 || index >= static_cast<int>(speces_.size())) {
      LOG_WARN("invalid argument. index=%d", index);
      return RC::INVALID_ARGUMENT;
    }
    spec = speces_[index].get();
    return RC::SUCCESS;
  }

  Record &record()
  {
    return *record_;
  }

  const Record &record() const
  {
    return *record_;
  }
private:
  Record *record_ = nullptr;
  const Table *table_ = nullptr;
  std::vector<std::shared_ptr<TupleCellSpec>> speces_;

  int null_map_;
  const FieldMeta *null_meta_;
};


class CompositeTuple : public Tuple
{
public:
  CompositeTuple() = default;
  virtual ~CompositeTuple()= default;
  
  int cell_num() const override{
    return cell_num_;
  }

  RC cell_at(int index, TupleCell &cell) const override{
    if (index < 0 || index >= static_cast<int>(cell_num_)) {
      return RC::GENERIC_ERROR;
    }
    if (tuples_.size() == 0) {
      return RC::GENERIC_ERROR;
    }

    for(Tuple *tuple: tuples_) {
      int newIdx = index - tuple->cell_num();
      if(newIdx < 0) {
        return tuple->cell_at(index,cell);
      }
      index = newIdx;
    }
    return RC::GENERIC_ERROR;
  }

  RC find_cell(const Field &field, TupleCell &cell) const override
  {
    RC rc = RC::SUCCESS;
    for(Tuple *tuple: tuples_) {
      rc = tuple->find_cell(field,cell);
      if(rc == SUCCESS) {
        return rc;
      }
    }
    return RC::NOTFOUND;
  }

  RC cell_spec_at(int index, const TupleCellSpec *&spec) const override
  {
    if (index < 0 || index >= static_cast<int>(cell_num_)) {
      return RC::GENERIC_ERROR;
    }
    if (tuples_.size() == 0) {
      return RC::GENERIC_ERROR;
    }
    for(Tuple *tuple: tuples_) {
      int newIdx = index - tuple->cell_num();
      if(newIdx < 0) {
        return tuple->cell_spec_at(index,spec);
      }
      index = newIdx;
    }
    return RC::GENERIC_ERROR;
  }

  void add_tuple(Tuple *tuple) {
    cell_num_ += tuple->cell_num();
    tuples_.push_back(tuple);
  }

  void clear() {
    cell_num_ = 0;
    tuples_.clear();
  }

private:
  int cell_num_ = 0;
  std::vector<Tuple *> tuples_;
};

class ProjectTuple : public Tuple
{
public:
  ProjectTuple() = default;
  virtual ~ProjectTuple()
  {
    for (TupleCellSpec *spec : speces_) {
      delete spec;
    }
    speces_.clear();
  }

  void set_tuple(Tuple *tuple)
  {
    this->tuple_ = tuple;
  }

  void add_cell_spec(TupleCellSpec *spec)
  {
    speces_.push_back(spec);
  }
  int cell_num() const override
  {
    return speces_.size();
  }

  RC cell_at(int index, TupleCell &cell) const override
  {
    if (index < 0 || index >= static_cast<int>(speces_.size())) {
      return RC::GENERIC_ERROR;
    }
    if (tuple_ == nullptr) {
      return RC::GENERIC_ERROR;
    }

    const TupleCellSpec *spec = speces_[index];
    return spec->expression()->get_value(*tuple_, cell);
  }

  RC find_cell(const Field &field, TupleCell &cell) const override
  {
    return tuple_->find_cell(field, cell);
  }
  
  RC cell_spec_at(int index, const TupleCellSpec *&spec) const override
  {
    if (index < 0 || index >= static_cast<int>(speces_.size())) {
      return RC::NOTFOUND;
    }
    spec = speces_[index];
    return RC::SUCCESS;
  }
private:
  std::vector<TupleCellSpec *> speces_;
  Tuple *tuple_ = nullptr;
};


class AggregationTuple : public Tuple
{
public:
  AggregationTuple() = default;
  virtual ~AggregationTuple()
  {
    for (TupleCellSpec *spec : speces_) {
      delete spec;
    }
    speces_.clear();
  }

  void set_schema(const std::vector<AggrField> &aggr_fields)
  {
    this->speces_.reserve(aggr_fields.size());
    for (const auto &field : aggr_fields) {
      speces_.push_back(new TupleCellSpec(new AggrExpr(field.table(), field.meta(),field.aggr_type())));
    }
  }

  void set_group_key(std::vector<TupleCell> &group_bys) {
    this->group_bys_ = group_bys;
  }

  void set_aggregate_value(std::vector<TupleCell> &aggregates) {
    this->aggregates_ = aggregates;
  }

  void set_aggregate_num(std::vector<size_t> &aggregates_num_) {
    this->aggregates_num_ = aggregates_num_;
  }

  void add_cell_spec(TupleCellSpec *spec)
  {
    speces_.push_back(spec);
  }
  int cell_num() const override
  {
    return speces_.size();
  }

  RC cell_at(int index, TupleCell &cell) const override
  {
    if (index < 0 || index >= static_cast<int>(speces_.size())) {
      return RC::GENERIC_ERROR;
    }
    auto expr = speces_[index]->expression();
    if(expr->type() == ExprType::FIELD) {
      cell = this->group_bys_[index];
    }else if(expr->type() == ExprType::AGGREGATE) {
      const AggrExpr * aggr_expr = (const AggrExpr *)expr;
      int i = index - group_bys_.size();
      cell = this->aggregates_[i];
      if(aggr_expr->aggr_type() == AVG_FUNC) {
        cell.Div(aggregates_num_[i]);
      }
    }else{
      return RC::GENERIC_ERROR;
    }

    return RC::SUCCESS;
  }

  RC find_cell(const Field &field, TupleCell &cell) const override
  {
    const char *table_name = field.table_name();
    const char *field_name = field.field_name();

    for (size_t i = 0; i < speces_.size(); ++i) {
      const AggrExpr * aggr_expr = (const AggrExpr *)speces_[i]->expression();
      if (0 == strcmp(table_name, aggr_expr->table_name()) 
        && 0 == strcmp(field_name, aggr_expr->field_name()) ) {
	      return cell_at(i, cell);
      }
    }
    return RC::NOTFOUND;
  }

  RC cell_spec_at(int index, const TupleCellSpec *&spec) const override
  {
    if (index < 0 || index >= static_cast<int>(speces_.size())) {
      return RC::NOTFOUND;
    }
    spec = speces_[index];
    return RC::SUCCESS;
  }
private:
  std::vector<TupleCellSpec *> speces_;
  std::vector<TupleCell> aggregates_;
  std::vector<TupleCell> group_bys_;
  std::vector<size_t> aggregates_num_;
};