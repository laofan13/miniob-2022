#include "common/log/log.h"
#include "sql/operator/descartes_operator.h"
#include "storage/record/record.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/common/field.h"

static void tuple_to_string(std::ostream &os, const Tuple &tuple)
{
  TupleCell cell;
  RC rc = RC::SUCCESS;
  bool first_field = true;
  for (int i = 0; i < tuple.cell_num(); i++) {
    rc = tuple.cell_at(i, cell);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to fetch field of cell. index=%d, rc=%s", i, strrc(rc));
      break;
    }

    if (!first_field) {
      os << " | ";
    } else {
      first_field = false;
    }
    cell.to_string(os);
  }
}

RC DescartesOperator::open()
{
  if (children_.size() < 1) {
    LOG_WARN("predicate operator must has more than one child");
    return RC::INTERNAL;
  }
  RC rc = RC::SUCCESS;
  for(auto oper: children_) {
    rc = oper->open();
    if(rc != RC::SUCCESS) {
      LOG_WARN("failed to open child operator: %s", strrc(rc));
      return rc;
    }
  }

  for(auto oper: children_) {
    std::vector<RowTuple > tuples;
    std::vector<Record > records;
    while (RC::SUCCESS == (rc = oper->next())) {
      Tuple *tuple = oper->current_tuple();
      if (nullptr == tuple) {
        rc = RC::INTERNAL;
        break;
      }
      RowTuple row_tuple(static_cast<RowTuple&>(*tuple));
      Record record(row_tuple.record());
      tuples.emplace_back(row_tuple);
      records.emplace_back(record);
    }
    dim_tuples_.emplace_back(tuples);
    dim_record_.emplace_back(records);
  }

  for(int i = 0;i < dim_tuples_.size();++i) {
    for(int j = 0;j < dim_tuples_[i].size();++j ) {
      dim_tuples_[i][j].set_record(&dim_record_[i][j]);
    }
  }

  // std::stringstream ss;
  // for(auto &vec: dim_tuples_) {
  //   for(auto &t:vec) {
  //     tuple_to_string(ss, t);
  //     ss << std::endl;
  //   }
  // }
  // LOG_WARN("dim_tuples_ %s",ss.str().c_str());

  std::vector<Tuple*> cur_result;
  descartes(0, cur_result);

  return RC::SUCCESS;
}

void DescartesOperator::descartes(int i, std::vector<Tuple*> cur_result){
  RC rc = RC::SUCCESS;
  if(i < dim_tuples_.size() - 1) {
    std::vector<RowTuple> &row_tuples = dim_tuples_[i];
    if(row_tuples.size() == 0) {
      return;
    }else{
      for(RowTuple &tuple: row_tuples) {
        cur_result.push_back(&tuple);
        descartes(i + 1,cur_result);
        cur_result.pop_back();
      }
    }
  }else if(i == dim_tuples_.size() - 1) {
    std::vector<RowTuple> &row_tuples = dim_tuples_[i];
    if(row_tuples.size() == 0) {
      return;
    }else{
      for(RowTuple &tuple: row_tuples) {
        cur_result.push_back(&tuple);
        descartes_result_.push_back(cur_result);
        cur_result.pop_back();
      }
    }
  }
}

RC DescartesOperator::next()
{
  RC rc = RC::SUCCESS;
  while(pos_ < descartes_result_.size()) {
    tuple_.clear_tuple();
    for(Tuple *tuple: descartes_result_[pos_]) {
       tuple_.add_tuple(tuple);
    }
    if (do_predicate(tuple_)) {
      pos_++;
      return rc;
    }
    pos_++;
  }
  return RC::INTERNAL;
}

RC DescartesOperator::close()
{
  for(auto oper: children_) {
    oper->close();
  }
  return RC::SUCCESS;
}

Tuple * DescartesOperator::current_tuple()
{
  // for(auto oper: children_) {
  //   Tuple *tuple = oper->current_tuple();
  //   tuple_.add_tuple(tuple);
  // }
  return &tuple_;
}

bool DescartesOperator::do_predicate(CompositeTuple &tuple)
{
  if (filter_stmt_ == nullptr || filter_stmt_->filter_units().empty()) {
    return true;
  }

  for (const FilterUnit *filter_unit : filter_stmt_->filter_units()) {
    Expression *left_expr = filter_unit->left();
    Expression *right_expr = filter_unit->right();
    CompOp comp = filter_unit->comp();
    TupleCell left_cell;
    TupleCell right_cell;

    if(left_expr->get_value(tuple, left_cell) != RC::SUCCESS) {
      continue;
    }
    if(right_expr->get_value(tuple, right_cell) != RC::SUCCESS) {
      continue;
    }

    const int compare = left_cell.compare(right_cell);
    bool filter_result = false;
    switch (comp) {
    case EQUAL_TO: {
      filter_result = (0 == compare); 
    } break;
    case LESS_EQUAL: {
      filter_result = (compare <= 0); 
    } break;
    case NOT_EQUAL: {
      filter_result = (compare != 0);
    } break;
    case LESS_THAN: {
      filter_result = (compare < 0);
    } break;
    case GREAT_EQUAL: {
      filter_result = (compare >= 0);
    } break;
    case GREAT_THAN: {
      filter_result = (compare > 0);
    } break;
    default: {
      LOG_WARN("invalid compare type: %d", comp);
    } break;
    }
    if (!filter_result) {
      return false;
    }
  }
  return true;
}