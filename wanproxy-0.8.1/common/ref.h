#ifndef	COMMON_REF_H
#define	COMMON_REF_H

template<typename T>
class Ref {
	class RefObj {
		T *ptr_;
		unsigned count_;
	public:
		RefObj(T *ptr)
		: ptr_(ptr),
		  count_(1)
		{ }

	private:
		~RefObj()
		{
			delete ptr_;
			ptr_ = NULL;
		}

	public:
		void hold(void)
		{
			count_++;
		}

		void drop(void)
		{
			if (count_-- == 1)
				delete this;
		}

		bool exclusive(void) const
		{
			return (count_ == 1);
		}

		T *get(void) const
		{
			return (ptr_);
		}
	};

	RefObj *obj_;
public:
	Ref(void)
	: obj_(NULL)
	{ }

	Ref(T *ptr)
	: obj_(NULL)
	{
		if (ptr != NULL)
			obj_ = new RefObj(ptr);
	}

	/*
	 * XXX Template...  See operator=.
	 */
	Ref(const Ref& ref)
	: obj_(ref.obj_)
	{
		if (obj_ != NULL)
			obj_->hold();
	}

	~Ref()
	{
		if (obj_ != NULL) {
			obj_->drop();
			obj_ = NULL;
		}
	}

	/*
	 * XXX
	 * Template so we can take a pointer from a Ref<> with a compatible
	 * base class?
	 */
	const Ref& operator= (const Ref& ref)
	{
		if (obj_ != NULL) {
			obj_->drop();
			obj_ = NULL;
		}

		if (ref.obj_ != NULL) {
			obj_ = ref.obj_;
			obj_->hold();
		}
		return (*this);
	}

	T *operator-> (void) const
	{
		return (obj_->get());
	}

	const T& operator* (void) const
	{
		const T *ptr = obj_->get();
		return (*ptr);
	}

	template<typename Tc>
	Tc cast(void) const
	{
		const T *ptr = obj_->get();
		return (dynamic_cast<Tc>(ptr));
	}

	bool exclusive(void) const
	{
		return (obj_->exclusive());
	}

	bool null(void) const
	{
		return (obj_ == NULL);
	}

	bool operator< (const Ref& b) const
	{
		return (obj_ < b.obj_);
	}
};

#endif /* !COMMON_REF_H */
