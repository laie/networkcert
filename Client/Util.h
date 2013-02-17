#pragma once
// boost-removed version of Util.h

#include <stdexcept>

#include <assert.h>

#include <algorithm>

#include <strstream>
#include <vector>

#pragma region Memory Leak Detector
#ifdef _DEBUG
#include <crtdbg.h>
namespace Util
{
	class DetectMemoryLeak
	{
	public:
		inline DetectMemoryLeak(int BreakPointNo=-1)
		{
			_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
			if ( BreakPointNo != -1 ) _CrtSetBreakAlloc( BreakPointNo );
		}
		inline ~DetectMemoryLeak()
		{
 		}
	};

}
#define DETECT_MEMORY_LEAK(BreakPointNo) namespace { ::Util::DetectMemoryLeak leakdetector(BreakPointNo); }
#else
namespace Util
{
	class DetectMemoryLeak
	{
	public:
		inline DetectMemoryLeak(int BreakPointNo=-1)
		{
			// on release mode, there's nothing to do
		}
		inline ~DetectMemoryLeak()
		{
		}
	};
}
#define DETECT_MEMORY_LEAK(BreakPointNo)
#endif
#pragma endregion


#pragma region Exception Control
	/*\  Message Throw
	Not deleted on release mode build
	\*/
	#define MTHROW(ExceptionType, Message, ...) \
		( \
			throw Util::ExceptionType##Exception( \
				L"[%s{%d}] " L ## Message, \
				__FUNCTIONW__, \
				__LINE__, \
				__VA_ARGS__ \
			), \
			false \
		)
	
	/*\  Assert Throw
	Not deleted on release mode build.
	\*/
	#define ATHROW(Expression) \
		( \
			(!!(Expression)) \
			|| ( \
				throw Util::AssertionException( \
					L"[%s{%d}] [checking expr. (%s)]", \
					__FUNCTIONW__, \
					__LINE__, \
					L ## #Expression \
				), \
				false) \
		)

	/*\  Assert Throw with Reason
	Not deleted on release mode build.
	\*/
	#define ATHROWR(Expression, Message, ...) \
		( \
			(!!(Expression)) \
			|| ( \
				throw Util::AssertionException( \
					L"[%s{%d}] [checking expr. (%s)], " L ## Message, \
					__FUNCTIONW__, \
					__LINE__, \
					L ## #Expression, \
					__VA_ARGS__ \
				), \
				false) \
		)

	/*\  Never Reach Throw
	Not deleted on release mode build.
	\*/
	#define NRTHROW() \
		( \
		throw Util::NeverReachException( \
			L"[%s{%d}] Not a designed behavior", \
			__FUNCTIONW__, \
			__LINE__ \
			), \
		false)

	// Breakpoint. Not deleted on release mode build
	#define BP() \
		( \
		__asm int 3; \
		)

	#ifdef _DEBUG
	//# define DMTHROW(ExceptionType, Message, ...) MTHROW(ExceptionType, Message, __VA_ARGS__)
	//# define DATHROW(Expression) ATHROW(Expression)
	//# define DATHROWR(Expression, Message, ...) ATHROWR(Expression, Message, __VA_ARGS__)
	//# define DNRTHROW() NRTHROW()
	//# define DBP() BP()
	# define DMTHROW MTHROW
	# define DATHROW ATHROW
	# define DATHROWR ATHROWR
	# define DNRTHROW NRTHROW
	# define DBP BP
	#else
	# define DMTHROW(ExceptionType, Message)
	# define DATHROW(Expression)
	# define DATHROWR(Expression, Message, ...)
	# define DNRTHROW()
	# define DBP()
	#endif
#pragma endregion

#pragma region Util Macros
	#define DECLDEF(Declaration) decltype(Declaration) Declaration
	#define COUNTOF(Expression) (sizeof((Expression))/sizeof(*(Expression)))
	#define REF_REDUCE(Var, NewName) decltype(Var) & NewName = Var;
	// Usage: WSTRCON("hi, this is the number one:" << 1)
	#define STRCON(Expression) (((std::stringstream&) \
						(std::stringstream() \
						<< Expression \
						) \
						).str())
	// Usage: WSTRCON(L"hi, this is the number one:" << 1)
	#define WSTRCON(Expression) (((std::wstringstream&) \
						(std::wstringstream() \
						<< Expression \
						) \
						).str())

	#define supportscope(...) std::identity<__VA_ARGS__>::type

	// support for sdecltype(something)::something
	#define sdecltype(...) supportscope(decltype(__VA_ARGS__))
#pragma endregion

namespace Util
{
	template<typename T>
	class instanceowner
	{
	public:
		T& content;
		inline instanceowner(T* OStrStream):
			content(*OStrStream)
		{ }
		inline ~instanceowner(){ delete &content; }
	};

}
#pragma endregion

namespace Util
{
#pragma region Traits
	template<typename T> struct DETONATE_MACRO_TYPE; 
	template<typename R, typename P1> struct DETONATE_MACRO_TYPE<R(P1)>
	{
		typedef P1 type; 
	};

	template<typename T>
	struct add_const_ptr
	{
		typedef typename std::add_pointer<typename std::add_const<typename std::remove_pointer<T>::type>::type>::type type;
	};
#pragma endregion
#pragma region Functions
	template<typename T>
	static inline typename T::off_type stream_size(T & Stream)
	{
		auto oldpos = Stream.tellg();
		Stream.seekg(0, std::ios::end);
		auto size = Stream.tellg();
		Stream.seekg(oldpos, std::ios::beg);
		return size;
	}
#pragma endregion
#pragma region Factory
	namespace Factory
	{
	}
#pragma endregion
#pragma region Smart ptr
	template<typename T>
	class UnwindArray
	{
	private:
		UnwindArray(const UnwindArray<T>&){ static_assert(false, "copy constructor"); }
	public:
		T *Arr;
		
		inline UnwindArray(){ Arr = NULL; }
		inline UnwindArray(T *Arr){ this->Arr = Arr; }
		inline ~UnwindArray(){ Set(NULL); }
		inline void Set(T *Arr){ if ( this->Arr ) delete[] this->Arr; this->Arr = Arr; }
  		inline T* operator=(T* A){ Set((T*)A); return Arr; }
		inline operator T*() const { return Arr; }
		inline T* operator()() const { return Arr; }
	};

	template<typename T>
	class UnwindItem
	{
	private:
		UnwindItem(const UnwindItem<T>&){ static_assert(false, "copy constructor"); }
	public:
		T *Item;

		inline UnwindItem(){ Item = NULL; }
		inline UnwindItem(T* Item){ this->Item = Item; }
		inline ~UnwindItem(){ Set(NULL); }
		inline void Set(T *Item){ if ( this->Item ) delete this->Item; this->Item = Item; }
		inline T* operator=(T* A){ Set(A); return Item; }
		inline operator T*() const { return Item; }
		inline T* operator()() const { return Item; }
		inline T* operator->() const { return Item; }
	};
#pragma endregion
#pragma region Exception
	enum EXCEPTION_TYPE // this is just for intellisense convenience
	{
		General = 0,
		NotImplemented,
		Unsupported,
		DirectXError,
		Win32Error,
		InvalidStatus,
		InvalidOperation,
		InvalidParameter,
		OutOfMemory,
		OutOfRange,
		ValueOverflow,
		ElementNotExisting,
		AccessDenied,
		NotFound,
		Assertion,
		NeverReach,
		OperationCanceled,
		EXCEPTION_TYPE_COUNT,
	};

	class BaseException : public virtual ::std::exception
	{
	protected:
		std::wstring	Message;
		std::string		MessageMB;

		inline void InitVAMsg(const std::wstring ExceptionName, const std::wstring & ExceptionMessage, va_list Args)
		{
			DWORD lenbuf = (DWORD)ExceptionMessage.size()+4096;
			UnwindArray<wchar_t> procstr		= new wchar_t[lenbuf];
			UnwindArray<wchar_t> procstrreflect	= new wchar_t[lenbuf];
			wcscpy_s(procstrreflect, lenbuf, ExceptionMessage.c_str());

			swprintf_s(procstr(), lenbuf, L"[Exception: %s] %s", ExceptionName.c_str(), procstrreflect);
			vswprintf_s(procstrreflect(), lenbuf, procstr(), Args);
			Message = std::wstring(procstrreflect());
			//MessageMB = strconv(Message);

			__if_exists(Log)
			{
				Log::WriteLog(L"%s", Message.c_str());
			}
		}

		inline BaseException(const std::wstring & ExceptionMessage) : Message(ExceptionMessage) { }
		inline BaseException(){ }


	public:
		virtual const char *what() const
		{ return MessageMB.c_str(); }
		virtual const wchar_t *GetMessage() const
		{ return Message.c_str(); }

	};

	#define DECLARE_EXCEPTION(ExceptionType) \
		class ExceptionType##Exception : public virtual BaseException \
		{ \
		public: \
			inline ExceptionType##Exception(const wchar_t *ExceptionMessage, ...) \
			{ \
				va_list vl; \
				va_start(vl, ExceptionMessage); \
				InitVAMsg(L## #ExceptionType, ExceptionMessage, vl); \
				va_end(vl); \
			} \
		}
	DECLARE_EXCEPTION(General);
	DECLARE_EXCEPTION(NotImplemented);
	DECLARE_EXCEPTION(Unsupported);
	DECLARE_EXCEPTION(DirectXError);
	DECLARE_EXCEPTION(Win32Error);
	DECLARE_EXCEPTION(InvalidStatus);
	DECLARE_EXCEPTION(InvalidOperation);
	DECLARE_EXCEPTION(InvalidParameter);
	DECLARE_EXCEPTION(OutOfMemory);
	DECLARE_EXCEPTION(OutOfRange);
	DECLARE_EXCEPTION(ValueOverflow);
	DECLARE_EXCEPTION(ElementNotExisting);
	DECLARE_EXCEPTION(AccessDenied);
	DECLARE_EXCEPTION(NotFound);
	DECLARE_EXCEPTION(Assertion);
	DECLARE_EXCEPTION(NeverReach);
	DECLARE_EXCEPTION(OperationCanceled);
#pragma endregion
#pragma region Property
	template<class C>
	class PropertyThisProvider;
	template<class C, typename T, T& (C::*GETPROC)() const, void (C::*SETPROC)( T const & )>
	class Property;
	template<class C, typename T>
	class PropertyBase;
	template<class C, typename T, T& (C::*GETPROC)() const, void (C::*SETPROC)( T const & )>
	class RProperty;
	template<class C, typename T, T& (C::*GETPROC)() const, void (C::*SETPROC)( T const & )>
	class WProperty;
	template<class C, typename T, T& (C::*GETPROC)() const, void (C::*SETPROC)( T const & )>
	class RWProperty;
	template<class CI, class CProperty>
	class PropertyInherit;

	
	template<typename C>
	class PropertyThisProvider // This must be declared on the parent class to provide right pointer to properties. Locate this on the top of all properties with private access
	{
	public:
		// Declaration of this will not cost an extra memory allocation per each instances.
		static C *This;
		inline PropertyThisProvider()
		{
			This =
				(C*)(
					(DWORD)this
					- (DWORD)(&((decltype(This))NULL)->__PropertyProvider__)
					);
		}
	};
	template<typename C> C *PropertyThisProvider<C>::This;

	
	template<class C, typename T>
	class PropertyBase // Property Base Class, Create PropertyBaseZero instead of this
	{
		friend C;
	protected:
		T &Value;
		C *Object;

	private:
		BYTE MemoryForValue[sizeof(T)];
		UnwindItem<T> UnwindableValue;
	public:
		template<typename TArchive>
		void serialize(TArchive & Archive, const unsigned int Version)
		{
			Archive & Value;
		}
	protected:

		inline void Constructor()
		{
			Object = (C*)( ((C*)(NULL))->__PropertyProvider__.This );
			memset(MemoryForValue, 0, sizeof(MemoryForValue));
		}

		inline PropertyBase()
			: Value(*(T*)MemoryForValue)
 		{
			Constructor();
			new ((T*)MemoryForValue) T;
		}
		
		// Initializing with created one. Use new keyword to make it. It will be managed.
		inline PropertyBase(T* ManagedInitializedValue)
			: Value(*ManagedInitializedValue)
		{
			Constructor();
			UnwindableValue = ManagedInitializedValue;
		}

		inline ~PropertyBase()
		{
			if ( UnwindableValue ) UnwindableValue = NULL;
			else
			{
				Value.~T();
				//delete (&MemoryForValue, &Value);
			}
		}
	};

	template<class C, typename T, T& (C::*GETPROC)() const, void (C::*SETPROC)( T const & )>
	class Property
		: public PropertyBase<C, T>
	{
	protected:
		inline Property(){ }
		inline Property(T* ValueInitialized)
			: PropertyBase<C, T>(ValueInitialized){ }

		inline const T& operator=(const T & A)
		{ (Object->*SETPROC)(std::forward<const T>(A)); return Get(); }
		inline const T& Set(const T & A)
		{ (Object->*SETPROC)(std::forward<const T>(A)); return Get(); }
		inline const T& operator+=(const T & A)
		{
			(Object->*SETPROC)(std::forward<T>((Object->*GETPROC)() += A));
			return Get();
		}
		inline const T& operator-=(const T & A)
		{
			(Object->*SETPROC)(std::forward<T>((Object->*GETPROC)() -= A));
			return Get();
		}
		inline const T& operator++()
		{
			(Object->*SETPROC)(std::forward<T>(++(Object->*GETPROC)()));
			return Get();
		}
		inline const T& operator--()
		{
			(Object->*SETPROC)(std::forward<T>(--(Object->*GETPROC)()));
			return Get();
		}

		inline operator const T& () const // a casting to target type
  		{ return (Object->*GETPROC)(); }
		inline const T& operator ()() const
		{ return (Object->*GETPROC)(); }
		inline const T& operator->() const
		{ return (Object->*GETPROC)(); }
		inline const T& Get() const
		{ return (Object->*GETPROC)(); }

	public:
		typedef C OwnerType;
		typedef T ValueType;
	};

	template<class C, typename T, T& (C::*GETPROC)() const, void (C::*SETPROC)( T const & )>
	class RProperty
		: public Property<C, T, GETPROC, SETPROC>
	{
		friend C;
	protected: // for 'friend' access
		inline RProperty(){ }
		inline RProperty(T* ValueInitialized)
			: Property<C, T, GETPROC, SETPROC>(ValueInitialized){ }

		using Property<C, T, GETPROC, SETPROC>::operator =;
		using Property<C, T, GETPROC, SETPROC>::Set;
		using Property<C, T, GETPROC, SETPROC>::operator ++;
		
		using Property<C, T, GETPROC, SETPROC>::operator --;
		using Property<C, T, GETPROC, SETPROC>::operator +=;
		using Property<C, T, GETPROC, SETPROC>::operator -=;

	public:
		using Property<C, T, GETPROC, SETPROC>::operator const T&;
		using Property<C, T, GETPROC, SETPROC>::operator ();
		using Property<C, T, GETPROC, SETPROC>::operator ->;
		using Property<C, T, GETPROC, SETPROC>::Get;

		template<class CI>
		inline operator PropertyInherit<CI, RProperty<C, T, GETPROC, SETPROC>> & () const
		{
			return (PropertyInherit<CI, RProperty<C, T, GETPROC, SETPROC>> &)*this;
		}
	};
	/*template<class C, typename T, typename T (C::*GETPROC)() const, typename TA>
	inline TA& operator=(TA& A, RProperty<C, T, GETPROC>& B)
	{
		return A = B();
	}*/

	template<class C, typename T, T& (C::*GETPROC)() const, void (C::*SETPROC)( T const & )>
	class WProperty
		: public Property<C, T, GETPROC, SETPROC>
	{
		friend C;
	protected: // for 'friend' access
		inline WProperty(){ }
		inline WProperty(T* ValueInitialized)
			: Property<C, T, GETPROC, SETPROC>(ValueInitialized){ }

		using Property<C, T, GETPROC, SETPROC>::operator const T&;
		using Property<C, T, GETPROC, SETPROC>::operator ();
		using Property<C, T, GETPROC, SETPROC>::operator ->;
		using Property<C, T, GETPROC, SETPROC>::Get;
	public:
		using Property<C, T, GETPROC, SETPROC>::operator =;
		using Property<C, T, GETPROC, SETPROC>::Set;
		using Property<C, T, GETPROC, SETPROC>::operator ++;
		using Property<C, T, GETPROC, SETPROC>::operator --;
		using Property<C, T, GETPROC, SETPROC>::operator +=;
		using Property<C, T, GETPROC, SETPROC>::operator -=;

		template<class CI>
		inline operator PropertyInherit<CI, WProperty<C, T, GETPROC, SETPROC>> & () const
		{
			return (PropertyInherit<CI, WProperty<C, T, GETPROC, SETPROC>> &)*this;
		}
	};

	template<class C, typename T, T& (C::*GETPROC)() const, void (C::*SETPROC)( const T & )>
	class RWProperty
		: public Property<C, T, GETPROC, SETPROC>
	{
		friend C;
	protected: // for 'friend' access
		inline RWProperty(){ }
		inline RWProperty(T* ValueInitialized)
			: Property<C, T, GETPROC, SETPROC>(ValueInitialized){ }
	public:
		using Property<C, T, GETPROC, SETPROC>::operator const T&;
		using Property<C, T, GETPROC, SETPROC>::operator ();
		using Property<C, T, GETPROC, SETPROC>::operator ->;
		using Property<C, T, GETPROC, SETPROC>::Get;

		using Property<C, T, GETPROC, SETPROC>::operator =;
		using Property<C, T, GETPROC, SETPROC>::Set;
		using Property<C, T, GETPROC, SETPROC>::operator ++;
		using Property<C, T, GETPROC, SETPROC>::operator --;
		using Property<C, T, GETPROC, SETPROC>::operator +=;
		using Property<C, T, GETPROC, SETPROC>::operator -=;

		template<class CI>
		inline operator PropertyInherit<CI, RWProperty<C, T, GETPROC, SETPROC>> & () const
		{
			return (PropertyInherit<CI, RWProperty<C, T, GETPROC, SETPROC>> &)*this;
		}
	};

	
	template<class CI, class C, typename T, T& (C::*GETPROC)() const, void (C::*SETPROC)( const T & )>
	class PropertyInherit<CI, RProperty<C, T, GETPROC, SETPROC>>
		: public RProperty<C, T, GETPROC, SETPROC>
	{
	private:
		typedef RProperty<C, T, GETPROC, SETPROC> CRProperty;
		friend CI;
		friend typename CRProperty::OwnerType;

		PropertyInherit(){ NRTHROW(); }
	protected:
		using CRProperty::operator =;
		using CRProperty::Set;
		using CRProperty::operator ++;
		using CRProperty::operator --;
		using CRProperty::operator +=;
		using CRProperty::operator -=;
	public:
		using CRProperty::operator const typename CRProperty::ValueType&;
		using CRProperty::operator ();
		using CRProperty::operator ->;
		using CRProperty::Get;
	};
	template<class CI, class C, typename T, T& (C::*GETPROC)() const, void (C::*SETPROC)( const T & )>
	class PropertyInherit<CI, WProperty<C, T, GETPROC, SETPROC>>
		: public WProperty<C, T, GETPROC, SETPROC>
	{
	private:
		typedef WProperty<C, T, GETPROC, SETPROC> CWProperty;
		friend CI;
		friend typename CWProperty::OwnerType;

		PropertyInherit(){ NRTHROW(); }
	protected:
		using CWProperty::operator const typename CWProperty::ValueType&;
		using CWProperty::operator ();
		using CWProperty::operator ->;
		using CWProperty::Get;
	public:
		using CWProperty::operator =;
		using CWProperty::Set;
		using CWProperty::operator ++;
		using CWProperty::operator --;
		using CWProperty::operator +=;
		using CWProperty::operator -=;
	};
	template<class CI, class C, typename T, T& (C::*GETPROC)() const, void (C::*SETPROC)( const T & )>
	class PropertyInherit<CI, RWProperty<C, T, GETPROC, SETPROC>>
		: public RWProperty<C, T, GETPROC, SETPROC>
	{
	private:
		typedef RWProperty<C, T, GETPROC, SETPROC> CRWProperty;
		friend CI;
		friend typename CRWProperty::OwnerType;

		PropertyInherit(){ NRTHROW(); }
	protected:
	public: // for 'friend' access
		using CRWProperty::operator const typename CRWProperty::ValueType&;
		using CRWProperty::operator ();
		using CRWProperty::operator ->;
		using CRWProperty::Get;

		using CRWProperty::operator =;
		using CRWProperty::Set;
		using CRWProperty::operator ++;
		using CRWProperty::operator --;
		using CRWProperty::operator +=;
		using CRWProperty::operator -=;
	};

	
	// Use this once in a class to use property functions. This should be above than other property declaration and private access.
	#define PROPERTY_PROVIDE(__ClassType) \
		friend Util::PropertyThisProvider<__ClassType>; \
		Util::PropertyThisProvider<__ClassType> __PropertyProvider__

	// Use this only in private access. Only DECLARE_PROPERTY should be shown
	#define DECLARE_PROP_TYPE_RW(__ClassType, Type, Name, GetterBody, SetterBody) \
		inline Util::DETONATE_MACRO_TYPE<void(Type)>::type& __Get##Name() const GetterBody; \
		inline void __Set##Name( Util::DETONATE_MACRO_TYPE<void(Type)>::type const & Value ) SetterBody; \
		typedef Util::RWProperty<__ClassType, Util::DETONATE_MACRO_TYPE<void(Type)>::type, &__ClassType::__Get##Name, &__ClassType::__Set##Name> Property##Name##__; \
		friend Property##Name##__; \
		friend Util::PropertyBase<__ClassType, Util::DETONATE_MACRO_TYPE<void(Type)>::type>

	// Use this only in private access. Only DECLARE_PROPERTY should be shown
	#define DECLARE_PROP_TYPE_R(__ClassType, Type, Name, GetterBody, SetterBody) \
		inline Util::DETONATE_MACRO_TYPE<void(Type)>::type& __Get##Name() const GetterBody; \
		inline void __Set##Name( Util::DETONATE_MACRO_TYPE<void(Type)>::type const & Value ) SetterBody; \
		typedef Util::RProperty<__ClassType, Util::DETONATE_MACRO_TYPE<void(Type)>::type, &__ClassType::__Get##Name, &__ClassType::__Set##Name> Property##Name##__; \
		friend Property##Name##__; \
		friend Util::PropertyBase<__ClassType, Util::DETONATE_MACRO_TYPE<void(Type)>::type>

	// Use this only in private access. Only DECLARE_PROPERTY should be shown
	#define DECLARE_PROP_TYPE_W(__ClassType, Type, Name, GetterBody, SetterBody) \
		inline Util::DETONATE_MACRO_TYPE<void(Type)>::type& __Get##Name() const GetterBody; \
		inline void __Set##Name( Util::DETONATE_MACRO_TYPE<void(Type)>::type const & Value ) SetterBody; \
		typedef Util::WProperty<__ClassType, Util::DETONATE_MACRO_TYPE<void(Type)>::type, &__ClassType::__Get##Name, &__ClassType::__Set##Name> Property##Name##__; \
		friend Property##Name##__; \
		friend Util::PropertyBase<__ClassType, Util::DETONATE_MACRO_TYPE<void(Type)>::type>

	// Actual declaration of a property
	#define DECLARE_PROPERTY(Name) Property##Name##__ Name

	// It's very nasty. but dunno how to improve.
	#define INHERIT_PROPERTY(__ClassType, Name) Util::PropertyInherit<__ClassType, Property##Name##__> & Name
	#define INHERIT_PROP_INIT(Name) Name(*(decltype(&Name))&__super::Name)
	
#pragma endregion
#pragma region OrderSafe
	// this must be statically used(set on zero-initialized memory)
	template<typename T>
	class OrderSafe
	{
	private:
		//UnwindItem<T> Object;
		T *pObject;

	public:
		inline OrderSafe(){ }
		inline ~OrderSafe(){ if ( pObject ) delete pObject; pObject = NULL; }

		inline operator T& ()
		{
			if ( !pObject ) pObject = new T;
			return *pObject;
		}
		inline T& operator ()()
		{
			if ( !pObject ) pObject = new T;
			return *pObject;
		}
		inline T& operator*()
		{
			if ( !pObject ) pObject = new T;
			return *pObject;
		}
	};
#pragma endregion
#pragma region Sync Issues
	class CriticalSection
	{
	public:
		CRITICAL_SECTION Cs;

		inline CriticalSection(){ InitializeCriticalSection(&Cs); }
		inline ~CriticalSection(){ DeleteCriticalSection(&Cs); }

		inline void Enter(){ EnterCriticalSection(&Cs); }
		inline bool TryEnter(){ return TryEnterCriticalSection(&Cs) != 0; }
		inline void Leave(){ LeaveCriticalSection(&Cs); }
	};

	class LockBlock
	{
	public:
		CRITICAL_SECTION *Cs;
		inline LockBlock(CRITICAL_SECTION & Cs)
		{
			this->Cs = &Cs;
			EnterCriticalSection(&Cs);
		}
		inline LockBlock(CriticalSection & Cs)
		{
			this->Cs = &Cs.Cs;
			EnterCriticalSection(&Cs.Cs);
		}
		inline ~LockBlock()
		{
			LeaveCriticalSection(Cs);
		}
	};

	class LockManual
	{
	public:
		CRITICAL_SECTION *Cs;
		int LockCounter;

		inline LockManual(CRITICAL_SECTION &Cs)
		{
			this->Cs = &Cs;
			LockCounter = 0;
		}
		inline LockManual(CriticalSection &Cs)
		{
			this->Cs = &Cs.Cs;
			LockCounter = 0;
		}
		inline ~LockManual()
		{
			while ( LockCounter-- ) LeaveCriticalSection(Cs);
		}

		inline void Enter()
		{
			LockCounter++;
			EnterCriticalSection(Cs);
		}

		inline bool TryEnter()
		{
			if ( TryEnterCriticalSection(Cs) )
			{
				LockCounter++;
				return true;
			} else return false;
		}

		inline void Leave()
		{
			LockCounter--;
			assert(LockCounter >= 0);
			LeaveCriticalSection(Cs);
		}
	};
	

#pragma endregion
#pragma region Data structures for special cases
	template<class C>
	class StepIterator
	{
		PROPERTY_PROVIDE(StepIterator<C>);

		DECLARE_PROP_TYPE_R(StepIterator<C>, int, Index, { return Index.Value; }, { Index.Value = Value; } );

		C *CurrentElement;
		
	public:
		DECLARE_PROPERTY(Index);
		

 		inline StepIterator()
		{
			CurrentElement = NULL;
			Index = -1;
		}

		inline void SetCurrentElement(int Index, C *Element)
		{
			assert(this->Index < Index);

			this->Index = Index;
			CurrentElement = Element;
		}

		inline operator C&()
		{ return *CurrentElement; }
		inline C& operator()()
		{ return *CurrentElement; }
		inline C& operator->()
		{ return *CurrentElement; }
	};

	// Use this data structure when random access speed is critically necessary and data's static
	template <typename T>
	class Queue
	{
	private:
		PROPERTY_PROVIDE(Queue);
		
		DECLARE_PROP_TYPE_R(Queue, int, Size, { return Size.Value; }, { Size.Value = Value; } );
		DECLARE_PROP_TYPE_R(Queue, int, Count, { return Count.Value; }, { Count.Value = Value; } );

		UnwindArray<T>		Arr;
		UnwindArray<BYTE>	ArrExisting;

	public:
		DECLARE_PROPERTY(Size);
		DECLARE_PROPERTY(Count);


		inline void Constructor()
		{
			Arr = 0;
			ArrExisting = 0;
			Size = 0;
			Count = 0;
		}

		inline Queue()
		{ Constructor(); }

		inline Queue(T *Arr, int Count)
		{
			Constructor();
			this->Arr = new T[Count];
			this->ArrExisting = new BYTE[Count];

			memcpy(this->Arr, Arr, sizeof(T)*Count);
			int i;
			for ( i=0; i < Count; i++ )
				ArrExisting[i] = true;
		}

		virtual ~Queue()
		{ }

		int Add(T Item)
		{
			if ( !this ) return -1;
			int i;
			for ( i=0; i < Size; i++ )
				if ( *(ArrExisting + i) == 0 ) break;

			if ( i == Size )
			{
				T *tmp = new T[Size + 100];
				memcpy(tmp, Arr, Size * sizeof(T));
				memset(tmp + Size, 0, 100 * sizeof(T));

				delete Arr;
				Arr = tmp;
		
				BYTE *tmpexisting = new BYTE[Size + 100];
				memcpy(tmpexisting, ArrExisting, Size * sizeof(BYTE));
				memset(tmpexisting + Size, 0, 100 * sizeof(BYTE));

				delete ArrExisting;
				ArrExisting = tmpexisting;

				Size += 100;
			}

			(Arr)[i] = Item;
			((BYTE*)ArrExisting)[i] = 1;
			++Count;
			return i;
		}
		void Add(Queue<T> *QueueAdd)
		{
			if ( !QueueAdd ) return;

			int i;
			DWORD count = 0;
			for ( i=0; i < QueueAdd->Size; i++ )
			{
				if ( QueueAdd->IsElementExisting(i) )
				{
					this->Add(QueueAdd->Get(i));
					count ++;
					if ( count == QueueAdd->Count ) break;
				}
			}
		}

		void Sub(int Index)
		{
			if ( !this ) return ;
			if ( !Arr ) return ;
			if ( Index < Size )
			{
				if ( ArrExisting[Index] )
				{
					--Count;
					Arr[Index] = 0;
					ArrExisting[Index] = 0;
				}
			}
		}

		void Clear()
		{
			Size = 0;
			Count = 0;
			Arr = NULL;
			ArrExisting = NULL;
		}

		inline bool IsElementExisting(int Index) const
		{
			if ( Index < 0 || Size <= Index  ) return false;
			return ArrExisting[Index] != 0;
		}
		inline T& Get(int Index) const
		{
			assert(IsElementExisting(Index));
			return Arr[Index];
		}
		inline T& operator[](int Index) const
		{ return Get(Index); }
		inline T* GetArray() const 
		{ return Arr; }

		inline bool Step(StepIterator<T> & IndexerElement)
		{
			int i;
			for ( i=IndexerElement.Index+1; i < Size && !IsElementExisting(i); i++ );
			if ( i < Size )
			{
				IndexerElement.SetCurrentElement(i, &Get(i));
				return true;
			} else
			{
				//IndexerElement.SetCurrentElement(-1, NULL);
				return false;
			}
		}
	};

	template<typename T, bool IsManaged = false>
	class CapacityStack
	{
	private:
		struct STACK_CHUNK
		{
			T *Arr;
			
			int Read;
			int Fill;

			STACK_CHUNK *Previous;
			STACK_CHUNK *Next;
		
			int Capacity;
		
			bool Add(T Element)
			{
				if ( Fill >= Capacity ) return false;
				Arr[Fill] = Element;
				Fill++;
				return true;
			}
		
			template<bool IsManaged> inline void IncRead(int Inc);
			template<> inline void IncRead<false>(int Inc)
			{ Read += Inc; }
			template<> inline void IncRead<true>(int Inc)
			{
				int i;
				for ( i=0; i < Inc; i++ )
					delete Arr[Read+i];
				Read += Inc;
			}
		
			inline STACK_CHUNK(int Capacity, STACK_CHUNK *Previous)
			{
				this->Arr = new T[Capacity];

				this->Read = 0;
				this->Fill = 0;
				
				this->Next		= NULL;
				this->Previous	= Previous;

				this->Capacity = Capacity;
			}

			template<bool IsManaged> inline void Destructor();
			template<> inline void Destructor<false>()
			{ delete Arr; }
			template<> inline void Destructor<true>()
			{
				int i;
				for ( i=Read; i < Fill; i++ )
					if ( Arr[i] ) delete Arr[i];
				Destructor<false>();
			}
			~STACK_CHUNK(){ Destructor<IsManaged>(); }
		};
	
		struct
		{
			int Capacity;
			
			STACK_CHUNK *FirstChunk;
			STACK_CHUNK *LastChunk;

			int TotalElementCount;
		} Internal;
		
		inline void Constructor(int Capacity)
		{
			if ( Capacity < 1 ) Capacity = 1;
			memset(&Internal, 0, sizeof(Internal));
			Internal.Capacity = Capacity;
		}
	public:
		inline CapacityStack()
		{ Constructor(1000); }
		inline CapacityStack(int Capacity)
		{ Constructor(Capacity); }
		~CapacityStack()
		{ Clear(); }

		// Pick data from first chunk by resultcount, and delete them
		int Pick(T *Result, int ResultCount)
		{
			//if ( !Result ) return 0;
			if ( !ResultCount ) return 0;

			if ( !Internal.FirstChunk ) return 0;
		
			int proceeded = 0;
			while ( proceeded < ResultCount )
			{
				if ( Internal.FirstChunk->Fill <= Internal.FirstChunk->Read )
				{
					if ( Internal.FirstChunk->Fill == Internal.Capacity )
					{
						STACK_CHUNK *next = Internal.FirstChunk->Next;
						delete Internal.FirstChunk;
						if ( Internal.FirstChunk == Internal.LastChunk )
							Internal.LastChunk = NULL;
						Internal.FirstChunk = next;
						if ( Internal.FirstChunk )
							Internal.FirstChunk->Previous = NULL;
					} else break;
				}
				if ( !Internal.FirstChunk ) break;
				int available = Internal.FirstChunk->Fill - Internal.FirstChunk->Read;
				if ( available + proceeded > ResultCount ) available = ResultCount-proceeded;
				if ( Result ) memcpy(
					Result + proceeded,
					Internal.FirstChunk->Arr + Internal.FirstChunk->Read,
					available*sizeof(T)
					);
				//Internal.FirstChunk->Read += available;
				Internal.FirstChunk->IncRead<IsManaged>(available);
				proceeded += available;
			}
			Internal.TotalElementCount -= proceeded;

			return proceeded;
		}

		inline int ReadFromStart(T *Result, int ResultCount)
		{ return ReadFrom(0, Result, ResultCount); }

		// Read from. If 'From' not starts with 0, it will be slower.
		int ReadFrom(int From, T *Result, int ResultCount)
		{
			if ( !Result ) return 0;
			if ( ResultCount <= 0 ) return 0;
			if ( From < 0 ) return 0;

			STACK_CHUNK *pchunk = Internal.FirstChunk;
			if ( !pchunk ) return 0;

			int proceeded = 0;
			while ( proceeded < ResultCount+From )
			{
				if ( !pchunk ) break;
				int available = pchunk->Fill - pchunk->Read;
				int fromoffset = 0;
				if ( proceeded - From < 0 ) fromoffset = From-proceeded;
				if ( available + proceeded - From > ResultCount ) available = ResultCount-proceeded+From;
				if ( available-fromoffset > 0 && pchunk->Read+fromoffset < pchunk->Fill )
				{
					memcpy(Result+proceeded-From+fromoffset,
						pchunk->Arr + pchunk->Read+fromoffset,
						(available-fromoffset)*sizeof(T)
						);
				}
				proceeded += available;
				pchunk = pchunk->Next;
			}
			int frommax = From;
			if ( frommax > GetElementCount() ) frommax = GetElementCount();
			return proceeded-frommax;
		}

		// Pop an element from last. Popped wouldn't be deleted(managed)
		T Pop()
		{
			if ( !Internal.LastChunk ) return NULL;
			if ( !Internal.LastChunk->Fill ) return NULL; // may not be needed
			if ( Internal.LastChunk->Fill <= Internal.LastChunk->Read ) return NULL;

			Internal.TotalElementCount--;
			T ret = Internal.LastChunk->Arr[Internal.LastChunk->Fill-1];
			Internal.LastChunk->Fill--;
			if ( !Internal.LastChunk->Fill )
			{
				STACK_CHUNK *oldlastchunk = Internal.LastChunk;
				Internal.LastChunk = Internal.LastChunk->Previous;
				delete oldlastchunk;
			}
			return ret;
		}
	
		// Push an element to the last
		bool Push(T Element)
		{
			if ( !Internal.LastChunk )
				Internal.LastChunk = new STACK_CHUNK(Internal.Capacity, NULL);
			if ( !Internal.FirstChunk ) Internal.FirstChunk = Internal.LastChunk;
			if ( Internal.LastChunk->Fill >= Internal.Capacity )
			{
				Internal.LastChunk->Next = new STACK_CHUNK(Internal.Capacity, Internal.LastChunk);
				Internal.LastChunk = Internal.LastChunk->Next;
			}
			bool ret = Internal.LastChunk->Add(Element);
			if ( ret )
			{
				Internal.TotalElementCount++;
			}
			return ret;
		}

		// Push an array of elements to the last
		bool Push(T *Array, int ArrayCount)
		{
			if ( !Array ) return false;
			if ( !ArrayCount ) return false;

			bool issucceeded = true;
			if ( !Internal.LastChunk )
				Internal.LastChunk = new STACK_CHUNK(Internal.Capacity, NULL);
			if ( !Internal.FirstChunk ) Internal.FirstChunk = Internal.LastChunk;
			int proceeded = 0;
			while ( proceeded < ArrayCount )
			{
				int available = (Internal.Capacity - Internal.LastChunk->Fill);
				if ( available <= 0 )
				{
					Internal.LastChunk->Next = new STACK_CHUNK(Internal.Capacity, Internal.LastChunk);
					Internal.LastChunk = Internal.LastChunk->Next;
					available = Internal.Capacity;
				}
				if ( available+proceeded > ArrayCount ) available = ArrayCount-proceeded;
				memcpy(Internal.LastChunk->Arr+Internal.LastChunk->Fill,
					Array+proceeded,
					available*sizeof(T));
				Internal.LastChunk->Fill += available;
				proceeded += available;
			}
			Internal.TotalElementCount += proceeded;
			return true;
		}

		// Paste an array of elements to the first
		bool Paste(T *Array, int ArrayCount)
		{
			if ( !Array ) return false;
			if ( !ArrayCount ) return false;
			bool issucceeded = true;
			if ( !Internal.LastChunk )
				Internal.LastChunk = new STACK_CHUNK(Internal.Capacity, NULL);
			if ( !Internal.FirstChunk ) Internal.FirstChunk = Internal.LastChunk;


			int start = Internal.FirstChunk->Read;
			int proceeded = 0;
			while ( proceeded < ArrayCount )
			{
				int available = Internal.FirstChunk->Read;
				if ( available <= 0 )
				{
					Internal.FirstChunk->Previous = new STACK_CHUNK(Internal.Capacity, NULL);
					Internal.FirstChunk->Previous->Next = Internal.FirstChunk;
					Internal.FirstChunk = Internal.FirstChunk->Previous;
					Internal.FirstChunk->Fill = Internal.Capacity;
					Internal.FirstChunk->Read = Internal.Capacity;
					available = Internal.FirstChunk->Read;
				}
				if ( available+proceeded > ArrayCount ) available = ArrayCount-proceeded;
				memcpy(Internal.FirstChunk->Arr+Internal.FirstChunk->Read-available,
					Array+ArrayCount-proceeded-available,
					available*sizeof(T));
				//Internal.FirstChunk->Read -= available;
				Internal.FirstChunk->IncRead<IsManaged>(-available);
				proceeded += available;
			}
			Internal.TotalElementCount += proceeded;
			return true;
		}

		inline int GetElementCount() const
		{ return Internal.TotalElementCount; }
	
		inline int GetCapacity() const
		{ return Internal.Capacity; }

		void Clear()
		{
			while ( Internal.LastChunk )
			{
				STACK_CHUNK *old = Internal.LastChunk;
				Internal.LastChunk = Internal.LastChunk->Previous;
				delete old;
			}
			Internal.TotalElementCount = 0;
			Internal.LastChunk = NULL;
			Internal.FirstChunk = NULL;
		}
	};

	template<typename T, bool IsManaged = false>
	class TriList
	{
	private:
		struct
		{
			int Capacity; // Must be larger than 1

			int CurrentBufferDepth;

			int CurrentSubCount;

			int TotalBufferAllocated;
			int TotalElementCount;

			int BiggestIndex;


			void	*Buffer;
			BYTE	*Existing;

			CapacityStack<int>		StackSubIndex;
		} Internal;

		inline void Constructor(int Capacity)
		{
			//memset(&Internal, 0, sizeof(Internal));
			Internal.Capacity = 0;
			Internal.CurrentBufferDepth = 0;
			Internal.CurrentSubCount = 0;
			Internal.TotalBufferAllocated = 0;
			Internal.TotalElementCount = 0;
			Internal.Buffer = 0;
			Internal.Existing = 0;
			Internal.BiggestIndex = -1;
			this->Internal.Capacity = Capacity; 
		}

		void Destructor()
		{ Clear(); }

		void AllocateOnce()
		{
			void **buf = 0;
			int i;
			int targetindex = Internal.TotalBufferAllocated;

			#pragma region Path finding
			UnwindArray<int> idxdepth;
			idxdepth = new int[Internal.CurrentBufferDepth+1];
			for ( i=0; i < Internal.CurrentBufferDepth+1; i++ )
			{
				idxdepth[Internal.CurrentBufferDepth-i]
					= targetindex % Internal.Capacity;
				targetindex /= Internal.Capacity;
			}
			#pragma endregion

			if ( targetindex )
			{
				#pragma region If a chunk of buffer should be allocated.. = depth should be increased
				void *beforebuffer = 0;
				int depth = Internal.CurrentBufferDepth;

				beforebuffer = Internal.Buffer;

				Internal.Buffer = (void*)new void*[Internal.Capacity];
				memset(Internal.Buffer, 0, sizeof(void*)*Internal.Capacity);
				((void**)Internal.Buffer)[0] = beforebuffer;
			
				if ( !depth )
				{
					buf = ((void***)Internal.Buffer)[1] = (void**)new T[Internal.Capacity];
					memset(buf, 0, sizeof(T)*Internal.Capacity);
				} else {
					buf = ((void***)Internal.Buffer)[1] = (void**)new void*[Internal.Capacity];
					memset(buf, 0, sizeof(void*)*Internal.Capacity);

					for ( ; depth-1 > 0; depth-- )
					{
						buf = ((void***)buf)[0]
							= new void*[Internal.Capacity];
						memset(buf, 0, sizeof(void*)*Internal.Capacity);
					}
					buf[0] = (void**)new T[Internal.Capacity];
					memset(buf[0], 0, sizeof(T)*Internal.Capacity);
				}
			
				depth = Internal.CurrentBufferDepth;
				beforebuffer = Internal.Existing;
				Internal.Existing = (BYTE*)new void*[Internal.Capacity];
				memset(Internal.Existing, 0, sizeof(void*)*Internal.Capacity);
				((void**)Internal.Existing)[0] = beforebuffer;
				if ( !depth )
				{
					buf = ((void***)Internal.Existing)[1] = (void**)new BYTE[(Internal.Capacity+7)/8];
					memset(buf, 0, (Internal.Capacity+7)/8);
				} else {
					buf = ((void***)Internal.Existing)[1] = (void**)new void*[Internal.Capacity];
					memset(buf, 0, sizeof(void*)*Internal.Capacity);
					for ( ; depth-1 > 0; depth-- )
					{
						buf = ((void***)buf)[0]
							= new void*[Internal.Capacity];
						memset(buf, 0, sizeof(void*)*Internal.Capacity);
					}
					buf[0] = (void**)new BYTE[(Internal.Capacity+7)/8];
					memset(buf[0], 0, (Internal.Capacity+7)/8);
				}

				Internal.CurrentBufferDepth++;
				#pragma endregion
			}
			else if ( !Internal.CurrentBufferDepth 
				&& !idxdepth[0] )
			{
				#pragma region The fist time allocation
				Internal.Buffer = new T[Internal.Capacity];
				memset(Internal.Buffer, 0, sizeof(T)*Internal.Capacity);

				Internal.Existing = new BYTE[(Internal.Capacity+7)/8];
				memset(Internal.Existing, 0, (Internal.Capacity+7)/8);
				#pragma endregion
			} else //else if ( idxdepth[0] < Internal.Capacity ) // of course always this is true
			{
				#pragma region When depth is not zero, depth should not be increased
				int depth = Internal.CurrentBufferDepth;

				buf = (void**)Internal.Buffer;
				if ( depth-1 == 0 )
				{
					#pragma region Put an allocated new buffer to the final
					buf[idxdepth[0]] = new T[Internal.Capacity];
					memset(buf[idxdepth[0]], 0, sizeof(T)*Internal.Capacity);
					#pragma endregion

				} else {
					#pragma region Get target chunk buffer and put a allocated new buffer to the final 
					for ( i=0; i < depth-1; i++ )
					{
						if ( !((void***)buf)[idxdepth[i]] )
						{
							buf = ((void***)buf)[idxdepth[i]]
							= new void*[Internal.Capacity];
							memset(buf, 0, sizeof(void*)*Internal.Capacity);
						} else {
							buf = ((void***)buf)[idxdepth[i]];
						}
					}
					buf[idxdepth[i]] = new T[Internal.Capacity];
					memset(buf[idxdepth[i]], 0, sizeof(T)*Internal.Capacity);
					#pragma endregion
				}

				depth = Internal.CurrentBufferDepth;
				buf = (void**)Internal.Existing;
				if ( depth-1 == 0 )
				{
					buf[idxdepth[0]] = new BYTE[(Internal.Capacity+7)/8];
					memset(buf[idxdepth[0]], 0, (Internal.Capacity+7)/8);
				} else {
					for ( i=0; i < depth-1; i++ )
					{
						if ( !((void***)buf)[idxdepth[i]] )
						{
							buf = ((void***)buf)[idxdepth[i]]
								= new void*[Internal.Capacity];
							memset(buf, 0, sizeof(void*)*Internal.Capacity);
						} else {
							buf = ((void***)buf)[idxdepth[i]];
						}
					}
					buf[idxdepth[i]] = new BYTE[(Internal.Capacity+7)/8];
					memset(buf[idxdepth[i]], 0, (Internal.Capacity+7)/8);
				}


			
				#pragma endregion
			}
		
			Internal.TotalBufferAllocated += Internal.Capacity;
		}

		template<bool IsManaged> inline void ProcDestructorBufferRCDeleteBuffer(void *Buffer, int Count, int IndexBase);
		template<> inline void ProcDestructorBufferRCDeleteBuffer<false>(void *Buffer, int Count, int IndexBase)
		{ if ( Buffer ) delete[] (T*)Buffer; }
		template<> inline void ProcDestructorBufferRCDeleteBuffer<true>(void *Buffer, int Count, int IndexBase)
		{
			if ( Buffer )
			{
				int i;
				for ( i=0; i < Count; i++ )
				{
					bool isexisting = IsElementExisting(IndexBase + i);
					if ( isexisting && ((T*)Buffer)[i] )
					{
						delete ((T*)Buffer)[i];
						((T*)Buffer)[i] = NULL;
					}
				}
			
			}
			if ( Buffer ) delete[] (T*)Buffer;
		}

		// Deletes Chunk buffer completely
		void ProcDestructorBufferRC(void *Buffer, int Depth, int *Cumulative)
		{
			int i;
			if ( Depth == Internal.CurrentBufferDepth )
			{
				int target = Internal.Capacity;
				if ( !Depth )
					target = Internal.TotalBufferAllocated;
			
				ProcDestructorBufferRCDeleteBuffer<IsManaged>(Buffer, target, *Cumulative);
				*Cumulative += target;
			} else 
			{
				int target = Internal.Capacity;
				for ( i=0; i < target; i++ )
				{
					ProcDestructorBufferRC(*((void**)Buffer+i), Depth+1, Cumulative);
					*((void**)Buffer+i) = 0;
					if ( *Cumulative >= Internal.TotalBufferAllocated ) break;
				}
				if ( Buffer ) delete[] Buffer;
			}
		}

		// Deletes IsExisting buffer completely
		void ProcDestructorExistingRC(void *Existing, int Depth, int *Cumulative)
		{
			int i;
			if ( Depth == Internal.CurrentBufferDepth )
			{
				int target = Internal.Capacity;
				if ( !Depth )
					target = Internal.TotalBufferAllocated;
				if ( Existing ) delete[] (BYTE*)Existing;
				*Cumulative += target;
			} else 
			{
				int target = Internal.Capacity;
				for ( i=0; i < target; i++ )
				{
					ProcDestructorExistingRC(*((void**)Existing+i), Depth+1, Cumulative);
					*((void**)Existing+i) = 0;
					if ( *Cumulative >= Internal.TotalBufferAllocated ) break;	
				}
				if ( Existing ) delete[] Existing;
			}
		}

		// Get current elements count. For test
		int ProcGetCurrentCountRC(void *Existing, int Depth, int *Cumulative)
		{
			int count = 0;
			int i;
			if ( Depth == Internal.CurrentBufferDepth )
			{
				int target = Internal.Capacity;
				if ( !Depth )
					target = Internal.TotalBufferAllocated;
				target = (target+7)/8;

				int cumi = 0;
				for ( i=0; i < target; i++ )
				{
					int totp = 8;
					if ( i == target-1 )
					{
						totp = Internal.Capacity % 8;
						if ( !totp ) totp = 8;
					}
					BYTE bit = *(BYTE*)((BYTE*)Existing+i);
					for ( int p=0; p < totp; p++ )
					{
						if ( ((bit >> p) & 0x01) ) count++;
					}
					cumi += totp;
				}
				*Cumulative += cumi;
			} else 
			{
				int target = Internal.Capacity;
				for ( i=0; i < target; i++ )
				{
					count += ProcGetCurrentCountRC(*((void**)Existing+i), Depth+1, Cumulative);
					if ( *Cumulative >= Internal.TotalBufferAllocated ) return count;
				}
			}
			return count;
		}

		// Get Empty Index RC
		int ProcGetAllocEmptyIndexRC(void *Existing, int Depth, int *Cumulative)
		{
			int i;
			if ( Depth == Internal.CurrentBufferDepth )
			{
				int target = Internal.Capacity;
				if ( !Depth )
					target = Internal.TotalBufferAllocated;
				target = (target+7)/8;

				int cumi = 0;
				for ( i=0; i < target; i++ )
				{
					int totp = 8;
					if ( i == target-1 )
					{
						totp = Internal.Capacity % 8;
						if ( !totp ) totp = 8;
					}
					BYTE bit = *(BYTE*)((BYTE*)Existing+i);
					for ( int p=0; p < totp; p++ )
						if ( !((bit >> p) & 0x01) )
							return *Cumulative+cumi+p;
					cumi += totp;
				}
				*Cumulative += cumi;
			} else 
			{
				int target = Internal.Capacity;
				for ( i=0; i < target; i++ )
				{
					int index;
					if ( -1 != (index = ProcGetAllocEmptyIndexRC(*((void**)Existing+i), Depth+1, Cumulative)) )
						return index;
					if ( *Cumulative >= Internal.TotalBufferAllocated ) return -1;
				}
			}
			return -1;
		}
		// Get Empty Index if no one's there, allocate a chunk of buffer
		int GetAllocEmptyIndex()
		{
			int cum = 0;
			int index = 0;
			if ( !Internal.CurrentSubCount )
			{
				index = Internal.TotalElementCount;
			} else 
			{
				if ( Internal.StackSubIndex.GetElementCount() )
				{
					index = Internal.StackSubIndex.Pop();
					#ifdef _DEBUG
					T *v;
					if ( GetElement(index, &v) ) throw std::bad_exception("[Debug]Element is not existing..");
					#endif
				} else index = ProcGetAllocEmptyIndexRC(Internal.Existing, 0, &cum);
			}
			if ( index == -1 )
			{
				AllocateOnce();
				return cum;
			} else {
				if ( index >= Internal.TotalBufferAllocated )
					AllocateOnce();
				return index;
			}
		}
		// Get pointer to the element by index
		bool GetPointerElement(int Index, T** pElement) const
		{
			if ( Index < 0 ) return false;
			if ( Index >= Internal.TotalBufferAllocated ) return false;
			if ( !pElement ) return false;

			int depthbuftmp[100];
			int i;
			void **buf = (void**)Internal.Buffer;

			UnwindArray<int> idxdepth;
			if ( Internal.CurrentBufferDepth+1 < 100 ) idxdepth = depthbuftmp;
			else idxdepth = new int[Internal.CurrentBufferDepth+1];

			for ( i=0; i < Internal.CurrentBufferDepth+1; i++ )
			{
				idxdepth[Internal.CurrentBufferDepth-i]
					= Index % Internal.Capacity;
				Index /= Internal.Capacity;
			}
		
			for ( i=0; i < Internal.CurrentBufferDepth; i++ )
				buf = (void**)buf[idxdepth[i]];
			*pElement = (T*)buf+idxdepth[i];

			if ( Internal.CurrentBufferDepth+1 < 100 ) idxdepth.Arr = NULL;
			return true;
		}
		// Get pointer to the Existing buffer by index
		bool GetPointerExisting(int Index, BYTE **pExisting, BYTE *BitIndex) const
		{
			if ( Index < 0 ) return false;
			if ( Index >= Internal.TotalBufferAllocated ) return false;
			if ( !pExisting ) return false;
			if ( !BitIndex ) return false;

			int depthbuftmp[100];
			int i;
			void **buf = (void**)Internal.Existing;

 			UnwindArray<int> idxdepth;
 			if ( Internal.CurrentBufferDepth+1 < 100 ) idxdepth = depthbuftmp;
			else idxdepth = new int[Internal.CurrentBufferDepth+1];

			for ( i=0; i < Internal.CurrentBufferDepth+1; i++ )
			{
				idxdepth[Internal.CurrentBufferDepth-i]
					= Index % Internal.Capacity;
				Index /= Internal.Capacity;
			}
		
			for ( i=0; i < Internal.CurrentBufferDepth; i++ )
			{
				buf = (void**)buf[idxdepth[i]];
			}
			*pExisting = (BYTE*)buf+(idxdepth[i])/8;
			*BitIndex  = idxdepth[i] % 8;

			if ( Internal.CurrentBufferDepth+1 < 100 ) idxdepth.Arr = NULL;
			return true;
		}
		
		// Set element by index
		bool SetElement(int Index, T Value)
		{
			if ( Index < 0 ) return false;
			if ( Index >= Internal.TotalBufferAllocated ) return false;
			
			T *pelement = 0;

			BYTE *pexisting = 0;
			BYTE bitindex = 0;
			if ( !GetPointerElement(Index, &pelement) ) return false;
			if ( !GetPointerExisting(Index, &pexisting, &bitindex) ) return false;

			*pelement = Value;
			
			if ( !(*pexisting & (0x01 << bitindex)) )
			{
				Internal.TotalElementCount ++;
				*pexisting |= (0x01 << bitindex);
				if ( Internal.CurrentSubCount > 0 )
					Internal.CurrentSubCount--;
				if ( Internal.BiggestIndex < Index )
					Internal.BiggestIndex = Index;
			}
			
			return true;
		}
		// Get element by index
		bool GetElement(int Index, T **Value) const
		{
			if ( Index < 0 ) return false;
			if ( Index >= Internal.TotalBufferAllocated ) return false;
			if ( !Value ) return false;

			BYTE *pexisting = 0;
			BYTE bitindex = 0;
			T *pelement = 0;
			if ( !GetPointerExisting(Index, &pexisting, &bitindex) )
			{
				//T null = {0};
				#ifdef _DEBUG
				throw std::bad_exception("[Debug]Element Pointer is not existing..");
				#endif
				//*Value = null;
				*Value = NULL;
				return false;
			}
			if ( !GetPointerElement(Index, &pelement) )
			{
				//T null = {0};
				#ifdef _DEBUG
				throw std::bad_exception("[Debug]Element Pointer is not existing..");
				#endif
				//*Value = null;
				*Value = NULL;
				return false;
			}

			if ( !(*pexisting & (0x01 << bitindex)) )
			{
				//T null = {0};
				//*Value = null;
				*Value = NULL;
				return false;
			}

			*Value = pelement;
			return true;
		}

		template<bool IsManaged> inline void SubSubManaged(int Index);
		template<> inline void SubSubManaged<false>(int Index)
		{
		};
		template<> inline void SubSubManaged<true>(int Index)
		{
			T *elem;
			if ( GetPointerElement(Index, &elem) && elem )
			{
				if ( *elem ) delete *elem;
				*elem = NULL;
			} else 
			{
				#ifdef _DEBUG
				throw std::bad_exception("[Debug]Element Pointer is not existing..");
				#endif
			}
		};

	public:
		//StepState StateStep;

		inline TriList()
		{ Constructor(1000); }
		inline TriList(int Capacity)
		{
			if ( Capacity <= 1 ) Capacity = 2;
			Constructor(Capacity);
		}

		virtual ~TriList()
		{ Destructor(); }

		inline DWORD GetTotalAllocated() const
		{ return Internal.TotalBufferAllocated; }

		inline DWORD GetElementCount() const
		{
			//int cum = 0;
			//int sizetmp = ProcGetCurrentCountRC(Internal.Existing, 0, &cum);
			//return sizetmp;
			return Internal.TotalElementCount;
		}

		inline DWORD GetBiggestIndex() const
		{ return Internal.BiggestIndex; }

		inline DWORD GetCapacity() const
		{ return Internal.Capacity; }

		int Add(T Element) // Returns Index
		{
			int index = GetAllocEmptyIndex();
			bool ret = SetElement(index, Element);
			#ifdef _DEBUG
			if ( !ret ) throw std::bad_exception("[Debug]Failed to set..");
			#endif
			return index;
		}

		bool Sub(int Index)
		{
			if ( !this ) return false;
			if ( Index < 0 ) return false;
			if ( Index >= Internal.TotalBufferAllocated ) return false;
			BYTE *pexisting = 0;
			BYTE bitindex = 0;
			bool ret = GetPointerExisting(Index, &pexisting, &bitindex);
			#ifdef _DEBUG
			if ( !ret )
				throw std::bad_exception("[Debug]Failed to get pointer..");
			#endif
			if ( !(*pexisting & (0x01 << bitindex)) ) return true;
			else
			{
				SubSubManaged<IsManaged>(Index);
				*pexisting &= ~(0x01 << bitindex);
				Internal.TotalElementCount--;
				Internal.CurrentSubCount++;
				Internal.StackSubIndex.Push(Index);
			}
		
			return true;
		}

		T & Get(int Index) const
		{
			T *ret;
			if ( !GetElement(Index, &ret) ) return *((T*)0);
			return *ret;
		}
		bool GetPointer(int Index, T **pElement) const
		{
			assert(Element);
			T *ret = NULL;
			if ( !GetElement(Index, &ret) )
			{
				*pElement = NULL;
				return false;
			}
			*pElement = ret;
			return true;
		}
		bool Get(int Index, T *Element) const
		{
			assert(Element);
			T *ret = NULL;
			if ( !GetElement(Index, &ret) )
			{
				memset(Element, 0, sizeof(T));
				return false;
			}
			memcpy(Element, ret, sizeof(T));
			return true;
		}

		bool Set(int Index, T Value)
		{
			if ( !IsElementExisting(Index) ) return false;
			return SetElement(Index, Value);
		}

		void PullFrom(int Index)
		{
			// TODO: Optimize
			if ( Index < 0 ) return;
			int i = Index, p;
			bool target = IsElementExisting(i);
			bool existing = false;
			int lastNull = Index;
			int lastExStart = Index;
			while ( i <= GetBiggestIndex() )
			{
				existing = target;
				for ( ; i <= GetBiggestIndex(); i++ )
				{
					existing = IsElementExisting(i);
					if ( existing != target )
						break;
				}
				if ( target )
				{
					if ( lastExStart != lastNull )
					for ( p=lastExStart; p < i; p++ )
					{
						bool issucceeded;
						T element = Get(p, &issucceeded);
						if ( !issucceeded ) MF::BreakExecution();
						if ( !SetElement(lastNull-lastExStart+p, element) ) MF::BreakExecution();
					}
				
					lastNull += i-lastExStart;
				} else lastExStart = i;

				target = existing;
			}
			for ( p=lastNull; p < i; p++ )
			{
				Sub(p);
			}
			Internal.StackSubIndex.Clear();
			if ( !Index )
			{
				for ( p=i-1; p >= lastNull; p-- )
				{
					Internal.StackSubIndex.Push(p);
				}
			} else {
				for ( p=Internal.BiggestIndex; p >= 0; p-- )
				{
					bool issucceeded = IsElementExisting(p);
					if ( !issucceeded ) Internal.StackSubIndex.Push(p);
				}
			}
		}
	
	
	
		bool IsElementExisting(int Index) const
		{
			BYTE *pexisting;
			BYTE bitindex;
			if ( !GetPointerExisting(Index, &pexisting, &bitindex) ) return false;
			if ( *pexisting & (1 << bitindex) ) return true;
			else return false;
		}

		void Clear()
		{
			int cum = 0;
			ProcDestructorBufferRC(this->Internal.Buffer, 0, &cum);
			this->Internal.Buffer = NULL;

			cum = 0;
			ProcDestructorExistingRC(this->Internal.Existing, 0, &cum);
			this->Internal.Existing = NULL;

			this->Internal.CurrentBufferDepth = 0;
			this->Internal.CurrentSubCount = 0;
			this->Internal.TotalBufferAllocated = 0;
			this->Internal.TotalElementCount = 0;
			this->Internal.BiggestIndex = -1;
			this->Internal.StackSubIndex.Clear();
		}

		/*T & operator[](int Index)
		{
			return Get(Index);
		}*/

		inline bool Step(StepIterator<T> & IndexerElement) const
		{
			/*const TriList<T, IsManaged> *_thistemp = this;
			TriList<T, IsManaged> *_this = (TriList<T, IsManaged>*)*(void**)&_thistemp;
			if ( !StateStep.IsStarted )
				_this->StateStep.Start();

			_this->StateStep.IncIndex();

			Element = NULL;*/

			T *element = NULL;
			DWORD i;

			for ( i=IndexerElement.Index+1; (int)i <= (int)GetBiggestIndex() && !GetElement(i, &element); i++ );
			if ( (int)i <= (int)GetBiggestIndex() )
			{
				IndexerElement.SetCurrentElement(i, element);
				return true;
			} else
			{
				//IndexerElement->SetCurrentElement(-1, NULL);
				return false;
			}
		}
	};
#pragma endregion
#pragma region Algorithms
	template<typename T>
	inline void Excg(T & A, T & B)
	{
		T temp = std::move(A);
		A = std::move(B);
		B = std::move(temp);
	}
	template<typename TIter, typename TGreater, typename TEqual>
	inline bool RCSortLarge(TIter Left, TIter Last, TGreater Greater, TEqual Equal, int Depth)
	{
	start:
		GreaterEqual =
			[&](decltype(*TIter) & A, decltype(*TIter) & B)
				{ return Greater(A, B) && Equal(A ,B); };
		LesserEqual =
			[&](decltype(*TIter) & A, decltype(*TIter) & B)
				{ return (!Greater(A, B)) && Equal(A ,B); };

		auto i = Left, p = Last-1;
		//T pivot = Arr[Left + Count/2];
		auto indexpivot = Left + Count/2;
		bool issorted = true;

		if ( Depth >= 100 ) return false; // Prevent for stackoverflow
		if ( i < 0 ) return true;
		if ( p-i <= 0 ) return true;

		for (;;)
		{
			for ( ; i <= p && GreaterEqual(*indexpivot, *p); p-- )
			{
				// 오른쪽부터 pivot보다 작은값을 찾아
				if ( p-1 >= Left && !GreaterEqual(*(p-1), *p) )
				{
					Excg(*(p-1), *p);
					//MF::Excg(Arr[p-1], Arr[p]);
					//if ( ArrIndex ) MF::Excg(ArrIndex[p-1], ArrIndex[p]);

					if ( p-1 == indexpivot ) indexpivot = p;
					else if ( p == indexpivot ) indexpivot = p-1;

					issorted = false;
					if ( !GreaterEqual(*indexpivot, *p) ) break;
					else
					{
						p--;
						continue;
					}
				}
			}
			if ( p-1 >= Left && !GreaterEqual(*(p-1), *p) ) issorted = false;

			for ( ; i <= p && LesserEqual(*indexpivot, *i); i++ )
			{
				// 왼쪽부터 pivot보다 큰값 찾아
				if ( issorted && i+1 < Left+Count && !Greater(*i, *(i+1)) )
					issorted = false;
			}
			if ( issorted && i+1 < Left+Count && !Greater(*i, *(i+1)) ) issorted = false;

			if ( i < p )
			{
				//교차 안했으면 바꾼다.
				Excg(*i, *p);
				//if ( ArrIndex ) Excg(*i, *p);

				if ( i == indexpivot ) indexpivot = p;
				else if ( p == indexpivot ) indexpivot = i;

			} else {
				if ( Left > p )
				{
					//교차했는데 p와 피봇이 다름. 피봇을 교환하고 break
					Excg(*Left, *indexpivot);
					//if ( ArrIndex ) Excg(*Left, *indexpivot);
				
					indexPivot = Left;

					Left++;
					Last--;
					goto start;
				}
				else if ( i >= Left + Count )
				{
					//Excg(*(Left+Count-1), *indexpivot);
					Excg(*(Last-1), *indexpivot);
					//if ( ArrIndex ) MF::Excg(ArrIndex[Left+Count-1], ArrIndex[indexPivot]);

					indexpivot = Left + Count - 1;

					Last--;
					goto start;

				}

				if ( issorted ) return true;
				if ( indexPivot < p )
				{
					Excg(*p, *indexPivot);
					if ( ArrIndex ) Excg(*p, *indexpivot);
					indexpivot = p;

					if ( !RCSortLarge(Left, p - Left, Depth + 1) ) return false;
					if ( !RCSortLarge(p + 1, Left+Count - p-1, Depth + 1) ) return false;
				} else {
					Excg(*(p+1), *indexpivot);
					if ( ArrIndex ) Excg(*(p+1), *indexpivot);
					indexpivot = p+1;

					if ( !RCSortLarge(Left, p - Left+1, Depth + 1) ) return false;
					if ( !RCSortLarge(p + 1+1, Left+Count - p-1-1, Depth + 1) ) return false;
				}
				return true;
			}
		}
	}

	template<typename TIter, typename TGreater, typename TEqual>
	inline void SortLarge(TIter First, TIter Last, TGreater Greater, TEqual Equal)
	{
		while ( !RCSortLarge(First, Last, Greater, Equal, 0) );
	}

	template<typename Iter, typename T>
	inline Iter binary_find(Iter begin, Iter end, const T& val)
	{
		// Finds the lower bound in at most log(last - first) + 1 comparisons
		Iter i = std::lower_bound(begin, end, val);

		if (i != end && *i == val)
			return i; // found
		else
			return end; // not found
	}

	template<typename Iter, typename T, typename TGreater>
	inline Iter binary_find(Iter begin, Iter end, const T& val, TGreater Greater)
	{
		// Finds the lower bound in at most log(last - first) + 1 comparisons
		Iter i = std::lower_bound(begin, end, val, Greater);

		// !greater() && greater() means val and *i equals together
		if (i != end && !Greater(*i, val) && Greater(*i, val) )
			return i; // found
		else
			return end; // not found
	}
#pragma endregion
}





