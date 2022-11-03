#include "common/log/log.h"
#include "sql/operator/join_operator.h"
#include "storage/record/record.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/common/field.h"


RC JoinOperator::open()
{
  for (auto it = scan_opers_.rbegin(); it != scan_opers_.rend(); it++)
  {
    auto &scan_oper = *it;
    std::vector<Tuple *> vec_tuples;
    std::vector<Record *> vec_records;
    RC rc = scan_oper->open();
    if (rc == RC::SUCCESS) {
      while (scan_oper->next() == RC::SUCCESS) {
        auto tuple = static_cast<RowTuple*>(scan_oper->current_tuple());

        Record *record = new Record(tuple->record());
        RowTuple *row_tuple = new RowTuple(*tuple);
        row_tuple->set_record(record);

        vec_records.push_back(record);
        vec_tuples.push_back(row_tuple);
      }
   
      index_mul_.push_back({vec_tuples.size(), total_num_});
      total_num_ *= vec_tuples.size();

      table_records_.push_back(vec_records);
      table_tuples_.push_back(vec_tuples);
      
    } else {
      return rc;
    }
  }

  return RC::SUCCESS;
}

RC JoinOperator::next()
{
  while(current_index_ < total_num_) {
    tuple_.clear();
    for (int i = 0; i < (int)table_tuples_.size(); i++)
    {
      auto &vec_tuples = table_tuples_[i];
      int row_index = (current_index_ / index_mul_[i].second) % index_mul_[i].first;
      auto tuple = vec_tuples[row_index];
      tuple_.add_tuple(tuple);
    }
    if(do_predicate(&tuple_)) {
      current_index_++;
      return RC::SUCCESS;
    }
    current_index_++;
  }
  return RC::RECORD_EOF;
}

RC JoinOperator::close()
{
  RC rc = RC::SUCCESS;
  for (auto scan_oper : scan_opers_) {
    if ((rc = scan_oper->close()) != RC::SUCCESS) {
      return rc;
    }
  }
  return rc;
}

Tuple * JoinOperator::current_tuple()
{
  return &tuple_;
}


bool JoinOperator::do_predicate(Tuple *tuple)
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

    if(left_expr->get_value(*tuple, left_cell) != RC::SUCCESS) {
      continue;
    }
    if(right_expr->get_value(*tuple, right_cell) != RC::SUCCESS) {
      continue;
    }

     // 有空值
    if(left_cell.attr_type() == NULLS || right_cell.attr_type() == NULLS) {
      if(left_cell.null_compare(right_cell,comp)) {
        continue;
      }
      return false;
    }

    int compare = left_cell.compare(right_cell);
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
    case LIKE_TO: {
      compare = left_cell.like_match(right_cell);
      filter_result = (0 == compare);
    } break;
    case NOT_LIKE: {
      compare = left_cell.like_match(right_cell);
      filter_result = (compare != 0);
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