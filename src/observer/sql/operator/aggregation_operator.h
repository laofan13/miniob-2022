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
// Created by WangYunlai on 2021/6/7.
//

#pragma once

#include "sql/operator/operator.h"
#include "sql/parser/parse.h"
#include "rc.h"

#include "util/hash_util.h"
/** AggregateKey represents a key in an aggregation operation */
struct AggregateKey {
  /** The group-by values */
  std::vector<TupleCell> group_bys_;
  /**
   * Compares two aggregate keys for equality.
   * @param other the other aggregate key to be compared with
   * @return `true` if both aggregate keys have equivalent group-by expressions, `false` otherwise
   */
  auto operator==(const AggregateKey &other) const -> bool {
    for (uint32_t i = 0; i < other.group_bys_.size(); i++) {
      if (group_bys_[i].compare(other.group_bys_[i]) != 0) {
        return false;
      }
    }
    return true;
  }
};

struct AggregateValue {
  std::vector<TupleCell> aggregates_;
  std::vector<size_t> aggregates_num_;
};

namespace std {
  template <>
  struct hash<AggregateKey> {
    auto operator()(const AggregateKey &agg_key) const -> std::size_t {
      size_t curr_hash = 0;
      for (const auto &key : agg_key.group_bys_) {
        if (!key.IsNull()) {
          curr_hash = HashUtil::CombineHashes(curr_hash, HashUtil::HashValue(key));
        }
      }
      return curr_hash;
    }
  };
};


class SimpleAggregationHashTable {
public:
  SimpleAggregationHashTable(const std::vector<AggrField> &aggr_fields)
  {
    this->agg_exprs_.reserve(aggr_fields.size());
    for (const auto &field : aggr_fields) {
      agg_exprs_.push_back(new AggrExpr(field.table(), field.meta(),field.aggr_type()));
    }
  }

  ~SimpleAggregationHashTable () {
    for (const auto exprs: agg_exprs_) {
      delete exprs;
    }
    agg_exprs_.clear();
  }

  /** @return The initial aggregrate value for this aggregation executor */
  auto GenerateInitialAggregateValue() -> AggregateValue {
    std::vector<TupleCell> values{};
    std::vector<size_t> aggrs_num{};
    for (const auto &aggr_expr : agg_exprs_) {
      AttrType attr_type = aggr_expr->field().attr_type();
      switch (aggr_expr->aggr_type()) {
      case AggrType::COUNT_FUNC:
        values.emplace_back(TupleCell::create_zero_cell(INTS));
        break;
      case AggrType::SUM_FUNC:
        values.emplace_back(TupleCell::create_zero_cell(attr_type));
        break;
      case AggrType::AVG_FUNC:
        values.emplace_back(TupleCell::create_zero_cell(attr_type));
        break;
      case AggrType::MAX_FUNC:
        values.emplace_back(TupleCell::create_min_cell(attr_type));
        break;
      case AggrType::MIN_FUNC:
        values.emplace_back(TupleCell::create_max_cell(attr_type));
        break;
      }
      aggrs_num.emplace_back(0);
    }
    return {values, aggrs_num};
  }

  /**
   * Combines the input into the aggregation result.
   * @param[out] result The output aggregate value
   * @param input The input value
   */
  void CombineAggregateValues(AggregateValue *result, const AggregateValue &input) {
    for (uint32_t i = 0; i < agg_exprs_.size(); i++) {
      switch (agg_exprs_[i]->aggr_type()) {
        case AggrType::COUNT_FUNC:
        result->aggregates_[i] = result->aggregates_[i].Add(1);
        break;
      case AggrType::SUM_FUNC:
        result->aggregates_[i] = result->aggregates_[i].Add(input.aggregates_[i]);
        break;
      case AggrType::AVG_FUNC:
        result->aggregates_[i] = result->aggregates_[i].Add(input.aggregates_[i]);
        break;
      case AggrType::MAX_FUNC:
        result->aggregates_[i] = result->aggregates_[i].Max(input.aggregates_[i]);
        break;
      case AggrType::MIN_FUNC:
        result->aggregates_[i] = result->aggregates_[i].Min(input.aggregates_[i]);
        break;
      }
      if(!input.aggregates_[i].IsNull()) {
        result->aggregates_num_[i]++;
      }
    }
  }

  /**
   * Inserts a value into the hash table and then combines it with the current aggregation.
   * @param agg_key the key to be inserted
   * @param agg_val the value to be inserted
   */
  void InsertCombine(const AggregateKey &agg_key, const AggregateValue &agg_val) {
    if (ht_.count(agg_key) == 0) {
      ht_.insert({agg_key, GenerateInitialAggregateValue()});
    }
    CombineAggregateValues(&ht_[agg_key], agg_val);
  }

   /** An iterator over the aggregation hash table */
  class Iterator {
   public:
    /** Creates an iterator for the aggregate map. */
    explicit Iterator(std::unordered_map<AggregateKey, AggregateValue>::const_iterator iter) : iter_{iter} {}

    /** @return The key of the iterator */
    auto Key() -> const AggregateKey & { return iter_->first; }

    /** @return The value of the iterator */
    auto Val() -> const AggregateValue & { return iter_->second; }

    /** @return The iterator before it is incremented */
    auto operator++() -> Iterator & {
      ++iter_;
      return *this;
    }

    /** @return `true` if both iterators are identical */
    auto operator==(const Iterator &other) -> bool { return this->iter_ == other.iter_; }

    /** @return `true` if both iterators are different */
    auto operator!=(const Iterator &other) -> bool { return this->iter_ != other.iter_; }

   private:
    /** Aggregates map */
    std::unordered_map<AggregateKey, AggregateValue>::const_iterator iter_;
  };

  /** @return Iterator to the start of the hash table */
  auto Begin() -> Iterator { return Iterator{ht_.cbegin()}; }

  /** @return Iterator to the end of the hash table */
  auto End() -> Iterator { return Iterator{ht_.cend()}; }

 private:
  std::unordered_map<AggregateKey, AggregateValue> ht_{};
  std::vector<const AggrExpr *> agg_exprs_;
};

class AggregationOperator : public Operator
{
public:
  AggregationOperator(const std::vector<AggrField> &aggr_fields, const std::vector<Field> &group_fields)
  :aggr_fields_(aggr_fields),
    group_fields_(group_fields),
    aht_(aggr_fields),
    aht_iterator_(aht_.Begin()) {
    }

  ~AggregationOperator() {
    if(children_[0]) {
      delete children_[0];
      children_[0] = nullptr;
    }
  }

  void add_projection(const Table *table, const FieldMeta *field_meta);
  void add_projection(const Table *table, const FieldMeta *field_meta, AggrType aggr_type);

  RC open() override;
  RC next() override;
  RC close() override;

  Tuple * current_tuple() override; 
  //int tuple_cell_num() const override
  //RC tuple_cell_spec_at(int index, TupleCellSpec &spec) const override
private:
  /** @return The tuple as an AggregateKey */
  auto MakeAggregateKey(const Tuple *tuple) -> AggregateKey {
    std::vector<TupleCell> keys;
    for (const auto &field : group_fields_) {
      TupleCell cell;
      tuple->find_cell(field, cell);
      keys.emplace_back(cell);
    }
    return {keys};
  }
  /** @return The tuple as an AggregateValue */
  auto MakeAggregateValue(const Tuple *tuple) -> AggregateValue {
    std::vector<TupleCell> vals;
    for (const auto &field : aggr_fields_) {
      TupleCell cell;
      tuple->find_cell(field, cell);
      vals.emplace_back(cell);
    }
    return {vals};
  }
private:
  const std::vector<AggrField> &aggr_fields_;
  const std::vector<Field> &group_fields_;

  SimpleAggregationHashTable aht_;
  SimpleAggregationHashTable::Iterator aht_iterator_;

  AggregationTuple tuple_;
};