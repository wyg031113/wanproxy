#ifndef	EVENT_TYPED_CONDITION_H
#define	EVENT_TYPED_CONDITION_H

#include <event/condition.h>
#include <event/typed_callback.h>

template<typename T>
class TypedCondition {
protected:
	TypedCondition(void)
	{ }

	virtual ~TypedCondition()
	{ }

public:
	virtual void signal(T) = 0;
	virtual Action *wait(TypedCallback<T> *) = 0;
};

template<typename T>
class TypedConditionVariable : public TypedCondition<T> {
	Action *wait_action_;
	TypedCallback<T> *wait_callback_;
public:
	TypedConditionVariable(void)
	: wait_action_(NULL),
	  wait_callback_(NULL)
	{ }

	~TypedConditionVariable()
	{
		ASSERT("/typed/condition/variable", wait_action_ == NULL);
		ASSERT("/typed/condition/variable", wait_callback_ == NULL);
	}

	void signal(T p)
	{
		if (wait_callback_ == NULL)
			return;
		ASSERT("/typed/condition/variable", wait_action_ == NULL);
		wait_callback_->param(p);
		wait_action_ = wait_callback_->schedule();
		wait_callback_ = NULL;
	}

	Action *wait(TypedCallback<T> *cb)
	{
		ASSERT("/typed/condition/variable", wait_action_ == NULL);
		ASSERT("/typed/condition/variable", wait_callback_ == NULL);

		wait_callback_ = cb;

		return (cancellation(this, &TypedConditionVariable::wait_cancel));
	}

private:
	void wait_cancel(void)
	{
		if (wait_callback_ != NULL) {
			ASSERT("/typed/condition/variable", wait_action_ == NULL);

			delete wait_callback_;
			wait_callback_ = NULL;
		} else {
			ASSERT("/typed/condition/variable", wait_action_ != NULL);

			wait_action_->cancel();
			wait_action_ = NULL;
		}
	}
};

#endif /* !EVENT_TYPED_CONDITION_H */
