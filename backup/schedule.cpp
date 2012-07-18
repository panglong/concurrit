/**
 * Copyright (c) 2010-2011,
 * Tayfun Elmas    <elmas@cs.berkeley.edu>
 * All rights reserved.
 * <p/>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * <p/>
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * <p/>
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * <p/>
 * 3. The names of the contributors may not be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 * <p/>
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "concurrit.h"

namespace concurrit {

/*
 * Yield Point
 */

YieldPoint::YieldPoint() {
	source_ = NULL;
	count_ = 1;
	loc_ = NULL;
	access_ = NULL;
	free_target_ = false;
	free_count_ = false;
	is_resolved_ = false;
	prev_ = NULL;
}

/********************************************************************************/

YieldPoint::YieldPoint(Coroutine* source,
		std::string& label, unsigned int count,
		SourceLocation* loc, SharedAccess* access,
		bool free_target, bool free_count) {
	safe_assert(source != NULL);
	source_ = source;
	label_ = label;
	count_ = count;
	loc_ = loc;
	access_ = access;
	free_target_ = free_target;
	free_count_ = free_count;
	is_resolved_ = true;
	prev_ = NULL;
}

/********************************************************************************/

YieldPoint::YieldPoint(std::string& source,
		std::string& label, unsigned int count,
		SourceLocation* loc, SharedAccess* access,
		bool free_target, bool free_count) {
	source_ = reinterpret_cast<Coroutine*>(new std::string(source));
	label_ = label;
	count_ = count;
	loc_ = loc;
	access_ = access;
	free_target_ = free_target;
	free_count_ = free_count;
	is_resolved_ = false;
	prev_ = NULL;
}

/********************************************************************************/

std::string YieldPoint::ToString() {
	std::stringstream s;
	if(loc_ != NULL) {
		s << "AT " << loc_->ToString() << "\n";
	}
	if(is_resolved_) {
		s << "(" << (source_ != NULL ? to_string(source_->tid()) : "NULL") << ", " << label_ << ", " << count_ << ", " << (access_ != NULL ? access_->ToString() : "<no access>") << ")";
	} else {
		s << "(" << (source_ != NULL ? reinterpret_cast<std::string*>(source_)->c_str() : "NULL") << ", " << label_ << ", " << count_ << ", " << (access_ != NULL ? access_->ToString() : "<no access>") << ")";
	}
	return s.str();
}

/********************************************************************************/

SchedulePoint* YieldPoint::Clone() {
	return new YieldPoint(source_, label_, count_, loc_ != NULL ? loc_->Clone() : NULL, access_ != NULL ? access_->Clone() : NULL, free_target_, free_count_);
}

/********************************************************************************/

point_t YieldPoint::Coverage() {
	safe_assert(IsResolved());
	point_t p(source_->tid(), label_, count_);
	return p;
}

/********************************************************************************/
/*
 * TransferPoint
 */

TransferPoint::TransferPoint() {
	yield_ = new YieldPoint();
	target_ = NULL;
	done_.clear();
	enabled_.clear();
	rem_count_ = yield_->count_;
	is_resolved_ = false;
	next_ = NULL;
}

TransferPoint::TransferPoint(YieldPoint* point, Coroutine* target) {
	yield_ = point;
	target_ = target;
	done_.clear();
	enabled_.clear();
	rem_count_ = yield_->count_;
	is_resolved_ = true;
	next_ = NULL;
}

TransferPoint::TransferPoint(YieldPoint* point, std::string& target) {
	yield_ = point;
	target_ = reinterpret_cast<Coroutine*>(new std::string(target));
	done_.clear();
	enabled_.clear();
	rem_count_ = yield_->count_;
	is_resolved_ = false;
	next_ = NULL;
}


/********************************************************************************/

void TransferPoint::Reset() {
	rem_count_ = safe_notnull(yield_)->count_;
	safe_assert(BETWEEN(1, rem_count_, yield_->count_));
	next_ = NULL;
	// NOTE: do not reset other fields, they need to be preserved across executions
	MYLOG(2) << "Reset done for transfer point: " << ToString();
}

/********************************************************************************/

bool TransferPoint::ConsumeOnce() {
	safe_assert(BETWEEN(1, rem_count_, yield_->count_));

	--rem_count_;
	return (rem_count_ == 0);
}

/********************************************************************************/

void TransferPoint::ConsumeAll() {
	safe_assert(BETWEEN(1, rem_count_, yield_->count_));

	rem_count_ = 0;
}

/********************************************************************************/
void TransferPoint::Load(Serializer* serializer) {
//	MYLOG(2) << "Loading point from file...";
//	yield_->source_ = reinterpret_cast<Coroutine*>(new std::string(serializer->Load<std::string>()));
//	target_ = reinterpret_cast<Coroutine*>(new std::string(serializer->Load<std::string>()));
//	yield_->label_ = serializer->Load<std::string>();
//	yield_->count_ = serializer->Load<int>();
//	MYLOG(2) << "Loaded point from file...";
}

/********************************************************************************/

void TransferPoint::Store(Serializer* serializer) {
	MYLOG(2) << "Storing point to file...";
	safe_assert(IsResolved());
	serializer->Store<ADDRINT>(yield_->source_->tid());
	serializer->Store<ADDRINT>(target_->tid());
	serializer->Store<std::string>(yield_->label_);
	serializer->Store<int>(yield_->count_);
	MYLOG(2) << "Stored point to file...";
}

/********************************************************************************/

std::string TransferPoint::ToString() {
	std::stringstream s;
	if(is_resolved_) {
		s << yield_->ToString() << " --> " << "(" << (target_ != NULL ? to_string(target_->tid()) : "NULL") << ", " << rem_count_ << ")";
	} else {
		s << yield_->ToString() << " --> " << "(" << (target_ != NULL ? reinterpret_cast<std::string*>(target_)->c_str() : "NULL") << ", " << rem_count_ << ")";
	}
	return s.str();
}

/********************************************************************************/

SchedulePoint* TransferPoint::Clone() {
	return new TransferPoint(yield_->Clone()->AsYield(), target_);
}

/********************************************************************************/

/*
 * ChoicePoint
 */

void ChoicePoint::SetAndConsumeCurrent() {
	if(ChoicePoint::GetCurrent(source_) == this) {
		safe_notnull(source_)->group()->scenario()->schedule()->ConsumeCurrent();
	} else {
		safe_notnull(source_)->group()->scenario()->schedule()->AddCurrent(this, true);
	}
}

ChoicePoint* ChoicePoint::GetCurrent(Coroutine* source /*=Coroutine::Current()*/) {
	safe_assert(source != NULL);
	// add the choice point to the current schedule
	Scenario* scenario = source->group()->scenario();
	SchedulePoint* current = scenario->schedule()->GetCurrent();
	if(current != NULL && current->IsChoice()) {
		safe_assert(safe_notnull(current->AsChoice())->source() == source);
		return safe_notnull(current->AsChoice());
	} else {
		return NULL;
	}
}

//ChoicePoint* ChoicePoint::GetOrMake(Chooser* chooser, Coroutine* source /*=Coroutine::Current()*/) {
//	safe_assert(source != NULL);
//	// add the choice point to the current schedule
//	Scenario* scenario = source->group()->scenario();
//	SchedulePoint* current = scenario->schedule()->GetCurrent();
//	if(current != NULL && current->IsChoice()) {
//		// reuse this choice point, otherwise
//		safe_assert(current->AsChoice()->source_ == source);
//		safe_assert(current->AsChoice()->chooser_ == chooser);
//	} else {
//		// create a new one
//		current = new ChoicePoint(chooser, source);
//		// add the point to the current schedule (do not consume yet, it is consumed below)
//		scenario->schedule()->AddCurrent(current, false);
//	}
//	// consume the current point
//	scenario->schedule()->ConsumeCurrent();
//	return current->AsChoice();
//}


/********************************************************************************/

/*
 * Schedule
 */

Schedule::Schedule() {
	filename_ = "";
	points_.clear();
	SetCurrentToFirst();
}

/********************************************************************************/

Schedule::Schedule(const char* filename, bool load /*= false*/) {
	filename_ = filename == NULL ? "" : std::string(filename);
	points_.clear();
	SetCurrentToFirst();
	if(load) {
		safe_assert(!filename_.empty());
		LoadFromFile();
	}
}

/********************************************************************************/

Schedule::~Schedule() {
	delete_points();
}

/********************************************************************************/

void Schedule::AddLast(SchedulePoint* point) {
	points_.push_back(point);
}

/********************************************************************************/

SchedulePoint* Schedule::RemoveLast() {
	SchedulePoint* point = GetLast();
	if(point != NULL) {
		points_.pop_back();
	}
	return point;
}

/********************************************************************************/

SchedulePoint* Schedule::GetLast() {
	if(Size() == 0) {
		return NULL;
	}
	return points_.back();
}

/********************************************************************************/

void Schedule::AddCurrent(SchedulePoint* point, bool consume) {
	safe_assert(BETWEEN(0, index_, Size()));

	if(index_ == Size()) {
		points_.push_back(point);
	} else {
		points_.insert(points_.begin()+index_, point);
	}

	// we also need to reset index to the end, do it
	if(consume) {
		index_++;
	}
}

/********************************************************************************/

bool Schedule::HasCurrent() {
	safe_assert(BETWEEN(0, index_, Size()));
	return index_ < Size();
}

/********************************************************************************/

SchedulePoint* Schedule::GetCurrent() {
	safe_assert(BETWEEN(0, index_, Size()));
	if(index_ == Size()) {
		// no point, return null
		return NULL;
	}
	SchedulePoint* point = points_[index_];
	safe_assert(point != NULL);
	safe_assert(point->IsTransfer() || point->IsChoice());

	return point;
}

/********************************************************************************/

void Schedule::ConsumeCurrent() {
	safe_assert(BETWEEN(0, index_, Size()-1));
	index_++;
}

/********************************************************************************/

void Schedule::SetCurrentToLast() {
	int sz = Size();
	safe_assert(0 <= sz);
	if(sz == 0) {
		index_ = 0;
	} else {
		index_ = sz - 1;
	}
}

/********************************************************************************/

void Schedule::SetCurrentToFirst() {
	index_ = 0;
}

/********************************************************************************/

size_t Schedule::Size() {
	return points_.size();
}

/********************************************************************************/

SchedulePoint* Schedule::RemoveCurrent() {
	SchedulePoint* point = GetCurrent();
	if(point != NULL) {
		points_.erase(points_.begin()+index_);
	}
	return point;
}

/********************************************************************************/

void Schedule::RemoveCurrentAndBeyond() {
	for (std::vector<SchedulePoint*>::iterator itr = points_.begin()+index_; itr < points_.end();) {
		itr = points_.erase(itr);
	}
	safe_assert(!HasCurrent());
	safe_assert(index_ == Size());
}

/********************************************************************************/

void Schedule::ClearUntakenPoints() {
	if(!points_.empty()) {
		for (std::vector<SchedulePoint*>::iterator itr = points_.begin(); itr < points_.end();) {
			SchedulePoint* point = *itr;
			safe_assert(point != NULL);
			if(point->IsTransfer()) {
				MYLOG(2) << __FUNCTION__ << " Resetting transfer point";
				safe_notnull(point->AsTransfer())->Reset();
				++itr;
			} else if(point->IsChoice()) {
				MYLOG(2) << __FUNCTION__ << " Skipping choice point";
				++itr; // go past it
			} else { // yield point
				MYLOG(2) << __FUNCTION__ << " Removing yield point";
				//delete point; // TODO(elmas): this causes a segmentation fault!
				itr = points_.erase(itr);
			}
		}
	}
	MYLOG(2) << __FUNCTION__ << " Cleared untaken points.";
}

/********************************************************************************/

void Schedule::Restart() {
	ClearUntakenPoints();
	SetCurrentToFirst();
	coverage_.Clear();
}

/********************************************************************************/

void Schedule::Clear() {
	// remove all schedule points
	delete_points();

	Restart();
}

/********************************************************************************/

Schedule* Schedule::Clone() {
	Schedule* schedule = new Schedule();
	(*schedule) = (*this);
	return schedule;
}

/********************************************************************************/

Schedule& Schedule::operator= (const Schedule& that) {
	if (this != &that) {
		delete_points();
		extend_points(const_cast<Schedule*>(&that));
		index_ = that.index_;
		coverage_ = that.coverage_;
		filename_ = that.filename_;
	}
	return *this;
}

/********************************************************************************/

void Schedule::delete_points() {
	for(std::vector<SchedulePoint*>::iterator itr = points_.begin(); itr < points_.end(); itr = points_.erase(itr)) {
		SchedulePoint* point = (*itr);
		safe_assert(point != NULL);
		delete point;
	}
	safe_assert(points_.empty());
}

/********************************************************************************/

void Schedule::extend_points(Schedule* that) {
	std::vector<SchedulePoint*>* that_points = &that->points_;
	for (std::vector<SchedulePoint*>::iterator itr = that_points->begin(); itr < that_points->end(); ++itr) {
		SchedulePoint* point = (*itr);
		this->AddLast(point->Clone());
	}
}

/********************************************************************************/

void Schedule::Load(Serializer* serializer) {
//	size_t len = serializer->Load<size_t>();
//	MYLOG(2) << "Will read " << len << " transfer points.";
//	for(size_t i = 0; i < len; ++i) {
//		try {
//			safe_assert(serializer->HasMore());
//			TransferPoint* point = new TransferPoint();
//			point->Load(serializer);
//			MYLOG(2) << "Loaded" << point->ToString() << " from file...";
//			// add point to our list
//			this->AddLast(point);
//		} catch(EOFException* eof) {
//			delete eof;
//			break;
//		}
//	}
//
//	Restart();
}

/********************************************************************************/


void Schedule::Store(Serializer* serializer) {
	serializer->Store<size_t>(Size());
	for (std::vector<SchedulePoint*>::iterator itr = points_.begin() ; itr < points_.end(); ++itr) {
		SchedulePoint* point = *itr;
		MYLOG(2) << "Saving " << point->ToString() << " to file...";
		point->Store(serializer);
	}
}

/********************************************************************************/

void Schedule::SaveToFile(const char* filename, bool append) {
	if(filename == NULL) {
		filename = filename_.c_str();
	}

	Serializer serializer(filename, append ? "a" : "w");
	Store(&serializer);
}

/********************************************************************************/

void Schedule::LoadFromFile(const char* filename) {
	if(filename == NULL) {
		filename = filename_.c_str();
	}

	Serializer serializer(filename, "r");
	Load(&serializer);
}

/********************************************************************************/

void Schedule::ToStream(const char* filename, bool append) {
	if(filename == NULL) {
		filename = filename_.c_str();
	}

	FILE* file = my_fopen(filename, append ? "a" : "w");

	fprintf(file, "\n***** Begin Schedule *****\n");
	fprintf(file, "Size: %d Index: %d\n", (int)Size(), (int)index_);
	unsigned int i = 0;
	for (std::vector<SchedulePoint*>::iterator itr = points_.begin() ; itr < points_.end(); ++itr) {
		SchedulePoint* point = *itr;
		safe_assert(point != NULL);
		if(i == index_) {
			fprintf(file, "CURRENT: ");
		}
		point->ToStream(file);
		fprintf(file, "\n\n");
		++i;
	}

	// print coverage
	fprintf(file, "Coverage:\n%s\n", coverage_.ToString().c_str());

	fprintf(file, "***** End Schedule *****\n");

	my_fclose(file);
}


/********************************************************************************/

std::string Schedule::ToString() {
	std::stringstream s;
	s << "\n***** Begin Schedule *****\n";
	s << "Size: " << Size() << " Index: " << index_ << "\n";
	unsigned int i = 0;
	for (std::vector<SchedulePoint*>::iterator itr = points_.begin() ; itr < points_.end(); ++itr) {
		SchedulePoint* point = *itr;
		safe_assert(point != NULL);
		if(i == index_) {
			s << "CURRENT: ";
		}
		s << point->ToString() << "\n\n";
		++i;
	}

	// print coverage
	s << "Coverage:\n" << coverage_.ToString() << "\n";

	s << "***** End Schedule *****\n";
	return s.str();
}

/********************************************************************************/

} // end namespace
