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

// class SimpleAggregationHashTable {
//  public:
 

//   SimpleAggregationHashTable(std::vector<const Expression *> &agg_exprs,
//                             const std::vector<QueryField> &aggr_fields)
//       : agg_exprs_{agg_exprs}, aggr_fields_{aggr_fields} {}

//   /** @return The initial aggregrate value for this aggregation executor */
//   auto GenerateInitialAggregateValue() -> AggregateValue {
//     std::vector<TupleCell> values{};
//     for (const auto &aggr_field : aggr_fields_) {
//       if(aggr_field.is_has_func()) {
//         switch (aggr_field.func_type()) {
//           case FuncType::COUNT_FUNC:
//             // Count starts at zero.
//             values.emplace_back(ValueFactory::GetIntegerValue(0));
//             break;
//           case FuncType::SUM_FUNC:
//             // Max starts at INT_MIN.
//             values.emplace_back(ValueFactory::GetIntegerValue(BUSTUB_INT32_MIN));
//             break;
//           case FuncType::SUM_FUNC:
//             // Max starts at INT_MIN.
//             values.emplace_back(ValueFactory::GetIntegerValue(BUSTUB_INT32_MIN));
//             break;
//           case FuncType::MAX_FUNC:
//             // Sum starts at zero.
//             values.emplace_back(ValueFactory::GetIntegerValue(0));
//             break;
//           case FuncType::MIN_FUNC:
//             // Min starts at INT_MAX.
//             values.emplace_back(ValueFactory::GetIntegerValue(BUSTUB_INT32_MAX));
//             break;
//         }
//       }
      
//     }
//     return {values};
//   }

//   /**
//    * Combines the input into the aggregation result.
//    * @param[out] result The output aggregate value
//    * @param input The input value
//    */
//   void CombineAggregateValues(AggregateValue *result, const AggregateValue &input) {
//     for (uint32_t i = 0; i < agg_exprs_.size(); i++) {
//       switch (agg_types_[i]) {
//         case AggregationType::CountAggregate:
//           // Count increases by one.
//           result->aggregates_[i] = result->aggregates_[i].Add(ValueFactory::GetIntegerValue(1));
//           break;
//         case AggregationType::SumAggregate:
//           // Sum increases by addition.
//           result->aggregates_[i] = result->aggregates_[i].Add(input.aggregates_[i]);
//           break;
//         case AggregationType::MinAggregate:
//           // Min is just the min.
//           result->aggregates_[i] = result->aggregates_[i].Min(input.aggregates_[i]);
//           break;
//         case AggregationType::MaxAggregate:
//           // Max is just the max.
//           result->aggregates_[i] = result->aggregates_[i].Max(input.aggregates_[i]);
//           break;
//       }
//     }
//   }

//   /**
//    * Inserts a value into the hash table and then combines it with the current aggregation.
//    * @param agg_key the key to be inserted
//    * @param agg_val the value to be inserted
//    */
//   void InsertCombine(const AggregateKey &agg_key, const AggregateValue &agg_val) {
//     if (ht_.count(agg_key) == 0) {
//       ht_.insert({agg_key, GenerateInitialAggregateValue()});
//     }
//     CombineAggregateValues(&ht_[agg_key], agg_val);
//   }

//  private:
//   /** The hash table is just a map from aggregate keys to aggregate values */
//   std::unordered_map<AggregateKey, AggregateValue> ht_{};
//   /** The aggregate expressions that we have */
//   std::vector<const Expression *> agg_exprs_;
//   /** The types of aggregations that we have */
//   const std::vector<QueryField> &aggr_fields_;
// };

class AggregationOperator : public Operator
{
public:
  AggregationOperator(const std::vector<AggrField> &sort_fields)
    : aggr_fields_(sort_fields)
  {
    
  }

  ~AggregationOperator(){
     if(children_[0]) {
      delete children_[0];
      children_[0] = nullptr;
    }
  }

  void add_agg_exprs(const std::vector<Field> &group_fields) {
    for(auto &field: group_fields) {
      agg_exprs_.push_back(new FieldExpr(field.table(),field.meta()));
    }
  }

  RC open() override;
  RC next() override;
  RC close() override;

  Tuple * current_tuple() override {
    return nullptr;
  }
  //int tuple_cell_num() const override
  //RC tuple_cell_spec_at(int index, TupleCellSpec &spec) const override
private:
  const std::vector<AggrField> &aggr_fields_;
  std::vector<const Expression *> agg_exprs_;
  
  // SimpleAggregationHashTable aht_;
};


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