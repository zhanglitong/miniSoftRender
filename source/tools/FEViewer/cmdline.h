
#pragma     once

#include    <iostream>
#include    <sstream>
#include    <vector>
#include    <map>
#include    <string>
#include    <stdexcept>
#include    <typeinfo>
#include    <cstring>
#include    <algorithm>
#ifdef __GNUC__
    #include <cxxabi.h>
#endif
#include    <cstdlib>

namespace   CMDLine
{
    /// 别名放在命名空间内,避免与 FE::String 等全局名字冲突
    using       String      =   std::string;
    using       Strings     =   std::vector<String>;
    namespace   DETAIL
    {
        template <typename Target, typename Source, bool Same>
        class   TLexicalCast
        {
        public:
            static Target cast(const Source &arg)
            {
                Target ret;
                std::stringstream ss;
                if (!(ss<<arg && ss>>ret && ss.eof()))
                    throw std::bad_cast();
                return ret;
            }
        };

        template <typename Target, typename Source>
        class   TLexicalCast<Target, Source, true>
        {
        public:
            static Target cast(const Source &arg)
            {
                return arg;
            }  
        };

        template <typename Source>
        class   TLexicalCast<String, Source, false>
        {
        public:
            static String cast(const Source &arg)
            {
                std::ostringstream ss;
                ss<<arg;
                return ss.str();
            }
        };

        template <typename Target>
        class   TLexicalCast<Target, String, false>
        {
        public:
            static Target cast(const String &arg)
            {
                Target ret;
                std::istringstream ss(arg);
                if (!(ss>>ret && ss.eof()))
                  throw std::bad_cast();
                return ret;
            }
        };

        template <typename T1, typename T2>
        struct  is_same
        {
            static const bool value = false;
        };

        template <typename T>
        struct  is_same<T, T>
        {
            static const bool value = true;
        };

        template<typename Target, typename Source>
        Target  lexical_cast(const Source &arg)
        {
            return TLexicalCast<Target, Source, DETAIL::is_same<Target, Source>::value>::cast(arg);
        }

        static inline String demangle(const String &name)
        {
        #ifdef __GNUC__
          int status=0;
          char *p=abi::__cxa_demangle(name.c_str(), 0, 0, &status);
          String ret(p);
          free(p);
          return ret;
        #else
            return  name;
        #endif
        }

        template <class T>
        String  readableTypename()
        {
            return demangle(typeid(T).name());
        }
        template <class T>
        String  defaultValue(T def)
        {
            return DETAIL::lexical_cast<String>(def);
        }
        template <>
        String  readableTypename<String>()
        {
          return "string";
        }

    } // DETAIL

    /// <summary>
    /// 
    /// </summary>
    class   CMDError : public std::exception
    {
    public:
        CMDError(const String &msg): msg(msg)
        {}
        ~CMDError() throw()
        {}
        const char *what() const throw() { return msg.c_str(); }
    private:
        String msg;
    };

    template <class T>
    struct  DefaultReader
    {
        T operator()(const String &str)
        {
            return DETAIL::lexical_cast<T>(str);
        }
    };
    /// <summary>
    /// 
    /// </summary>
    /// <typeparam name="T"></typeparam>
    template <class T>
    struct  RangeReader
    {
        RangeReader(const T &low, const T &high)
        : low(low), high(high)
        {}
        T operator()(const String &s) const
        {
          T ret=DefaultReader<T>()(s);
          if (!(ret>=low && ret<=high)) throw CMDLine::CMDError("range_error");
          return ret;
        }
    private:
      T low, high;
    };

    template <class T>
    RangeReader<T> range(const T &low, const T &high)
    {
      return RangeReader<T>(low, high);
    }
    /// <summary>
    /// 
    /// </summary>
    /// <typeparam name="T"></typeparam>
    template <class T>
    struct  OneOfReader
    {
      T operator()(const String &s)
      {
        T ret=DefaultReader<T>()(s);
        if (std::find(alt.begin(), alt.end(), ret)==alt.end())
          throw CMDError("");
        return ret;
      }
      void add(const T &v){ alt.push_back(v); }
    private:
      std::vector<T> alt;
    };
    /// <summary>
    /// 
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="a1"></param>
    /// <returns></returns>
    template <class T>
    OneOfReader<T> oneof(T a1)
    {
      OneOfReader<T> ret;
      ret.add(a1);
      return ret;
    }
    /// <summary>
    /// 
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="a1"></param>
    /// <param name="a2"></param>
    /// <returns></returns>
    template <class T>
    OneOfReader<T> oneof(T a1, T a2)
    {
      OneOfReader<T> ret;
      ret.add(a1);
      ret.add(a2);
      return ret;
    }
    /// <summary>
    /// 
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="a1"></param>
    /// <param name="a2"></param>
    /// <param name="a3"></param>
    /// <returns></returns>
    template <class T>
    OneOfReader<T> oneof(T a1, T a2, T a3)
    {
      OneOfReader<T> ret;
      ret.add(a1);
      ret.add(a2);
      ret.add(a3);
      return ret;
    }
    /// <summary>
    /// 
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="a1"></param>
    /// <param name="a2"></param>
    /// <param name="a3"></param>
    /// <param name="a4"></param>
    /// <returns></returns>
    template <class T>
    OneOfReader<T> oneof(T a1, T a2, T a3, T a4)
    {
      OneOfReader<T> ret;
      ret.add(a1);
      ret.add(a2);
      ret.add(a3);
      ret.add(a4);
      return ret;
    }
    /// <summary>
    /// 
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="a1"></param>
    /// <param name="a2"></param>
    /// <param name="a3"></param>
    /// <param name="a4"></param>
    /// <param name="a5"></param>
    /// <returns></returns>
    template <class T>
    OneOfReader<T> oneof(T a1, T a2, T a3, T a4, T a5)
    {
      OneOfReader<T> ret;
      ret.add(a1);
      ret.add(a2);
      ret.add(a3);
      ret.add(a4);
      ret.add(a5);
      return ret;
    }
    /// <summary>
    /// 
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="a1"></param>
    /// <param name="a2"></param>
    /// <param name="a3"></param>
    /// <param name="a4"></param>
    /// <param name="a5"></param>
    /// <param name="a6"></param>
    /// <returns></returns>
    template <class T>
    OneOfReader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6)
    {
      OneOfReader<T> ret;
      ret.add(a1);
      ret.add(a2);
      ret.add(a3);
      ret.add(a4);
      ret.add(a5);
      ret.add(a6);
      return ret;
    }
    /// <summary>
    /// /// <summary>
    /// 
    /// </summary>
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="a1"></param>
    /// <param name="a2"></param>
    /// <param name="a3"></param>
    /// <param name="a4"></param>
    /// <param name="a5"></param>
    /// <param name="a6"></param>
    /// <param name="a7"></param>
    /// <returns></returns>
    template <class T>
    OneOfReader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6, T a7)
    {
      OneOfReader<T> ret;
      ret.add(a1);
      ret.add(a2);
      ret.add(a3);
      ret.add(a4);
      ret.add(a5);
      ret.add(a6);
      ret.add(a7);
      return ret;
    }
    /// <summary>
    /// 
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="a1"></param>
    /// <param name="a2"></param>
    /// <param name="a3"></param>
    /// <param name="a4"></param>
    /// <param name="a5"></param>
    /// <param name="a6"></param>
    /// <param name="a7"></param>
    /// <param name="a8"></param>
    /// <returns></returns>
    template <class T>
    OneOfReader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6, T a7, T a8)
    {
      OneOfReader<T> ret;
      ret.add(a1);
      ret.add(a2);
      ret.add(a3);
      ret.add(a4);
      ret.add(a5);
      ret.add(a6);
      ret.add(a7);
      ret.add(a8);
      return ret;
    }
    /// <summary>
    /// 
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="a1"></param>
    /// <param name="a2"></param>
    /// <param name="a3"></param>
    /// <param name="a4"></param>
    /// <param name="a5"></param>
    /// <param name="a6"></param>
    /// <param name="a7"></param>
    /// <param name="a8"></param>
    /// <param name="a9"></param>
    /// <returns></returns>
    template <class T>
    OneOfReader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6, T a7, T a8, T a9)
    {
      OneOfReader<T> ret;
      ret.add(a1);
      ret.add(a2);
      ret.add(a3);
      ret.add(a4);
      ret.add(a5);
      ret.add(a6);
      ret.add(a7);
      ret.add(a8);
      ret.add(a9);
      return ret;
    }
    /// <summary>
    /// 
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="a1"></param>
    /// <param name="a2"></param>
    /// <param name="a3"></param>
    /// <param name="a4"></param>
    /// <param name="a5"></param>
    /// <param name="a6"></param>
    /// <param name="a7"></param>
    /// <param name="a8"></param>
    /// <param name="a9"></param>
    /// <param name="a10"></param>
    /// <returns></returns>
    template <class T>
    OneOfReader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6, T a7, T a8, T a9, T a10)
    {
      OneOfReader<T> ret;
      ret.add(a1);
      ret.add(a2);
      ret.add(a3);
      ret.add(a4);
      ret.add(a5);
      ret.add(a6);
      ret.add(a7);
      ret.add(a8);
      ret.add(a9);
      ret.add(a10);
      return ret;
    }

    /// <summary>
    /// 
    /// </summary>
    class   Parser
    {
    public:
        Parser()
        {}
        ~Parser()
        {
          for (std::map<String, OptionBase*>::iterator p=_options.begin();
               p!=_options.end(); p++)
            delete p->second;
        }

        void add(const String &name,
                 char short_name=0,
                 const String &desc=""){
          if (_options.count(name)) throw CMDError("multiple definition: "+name);
          _options[name]=new OptionWithoutValue(name, short_name, desc);
          _ordered.push_back(_options[name]);
        }

        template <class T>
        void add(const String &name,
                 char short_name=0,
                 const String &desc="",
                 bool need=true,
                 const T def=T()){
          add(name, short_name, desc, need, def, DefaultReader<T>());
        }

        template <class T, class F>
        void add(const String &name,
                 char short_name=0,
                 const String &desc="",
                 bool need=true,
                 const T def=T(),
                 F reader=F())
        {
            if (_options.count(name)) throw CMDError("redefinition: "+name);
            _options[name]=new OptionWithValueWithReader<T, F>(name, short_name, need, def, desc, reader);
            _ordered.push_back(_options[name]);
        }

      void footer(const String &f){
        _ftr=f;
      }

      void set_program_name(const String &name)
      {
        _progName=name;
      }

      bool exist(const String &name) const
      {
        if (_options.count(name)==0) throw CMDError("no flag: --"+name);
        return _options.find(name)->second->has_set();
      }

      template <class T>
      const T &get(const String &name) const
      {
        if (_options.count(name)==0) throw CMDError("no flag: --"+name);
        const OptionWithValue<T> *p=dynamic_cast<const OptionWithValue<T>*>(_options.find(name)->second);
        if (p==NULL) throw CMDError("type mismatch '"+name+"'");
        return p->get();
      }

      const Strings &rest() const
      {
        return _others;
      }

      bool parse(const String &arg)
      {
        Strings args;

        String buf;
        bool in_quote=false;
        for (String::size_type i=0; i<arg.length(); i++){
          if (arg[i]=='\"'){
            in_quote=!in_quote;
            continue;
          }

          if (arg[i]==' ' && !in_quote)
          {
            args.push_back(buf);
            buf =   "";
            continue;
          }
          if (arg[i]=='\\')
          {
            i++;
            if (i>=arg.length()){
              _errors.push_back("unexpected occurrence of '\\' at end of string");
              return false;
            }
          }

          buf+=arg[i];
        }

        if (in_quote)
        {
          _errors.push_back("quote is not closed");
          return false;
        }

        if (buf.length()>0)
          args.push_back(buf);

        for (size_t i=0; i<args.size(); i++)
          std::cout<<"\""<<args[i]<<"\""<<std::endl;

        return parse(args);
      }

      bool parse(const Strings &args)
      {
        /// 没有任何参数时视为解析成功:由调用方决定是否打印用法
        if (args.empty())
        {
          _errors.clear();
          _others.clear();
          return true;
        }
        int argc=static_cast<int>(args.size());
        std::vector<const char*> argv(argc);

        for (int i=0; i<argc; i++)
          argv[i]=args[i].c_str();

        return parse(argc, &argv[0]);
      }

      bool parse(int argc, const char * const argv[])
      {
        _errors.clear();
        _others.clear();

        if (argc<1){
          _errors.push_back("argument number must be longer than 0");
          return false;
        }
        if (_progName=="")
          _progName=argv[0];

        std::map<char, String> lookup;
        for (std::map<String, OptionBase*>::iterator p=_options.begin();
             p!=_options.end(); p++){
          if (p->first.length()==0) continue;
          char initial=p->second->short_name();
          if (initial){
            if (lookup.count(initial)>0)
            {
              lookup[initial]="";
              _errors.push_back(String("short option '")+initial+"' is ambiguous");
              return false;
            }
            else
                lookup[initial]=p->first;
          }
        }

        for (int i=1; i<argc; i++)
        {
          if (strncmp(argv[i], "--", 2)==0)
          {
            const char *p=strchr(argv[i]+2, '=');
            if (p)
            {
              String name(argv[i]+2, p);
              String val(p+1);
              set_option(name, val);
            }
            else
            {
              String name(argv[i]+2);
              if (_options.count(name)==0)
              {
                _errors.push_back("undefined option: --"+name);
                continue;
              }
              if (_options[name]->has_value())
              {
                if (i+1>=argc)
                {
                  _errors.push_back("option needs value: --"+name);
                  continue;
                }
                else
                {
                  i++;
                  set_option(name, argv[i]);
                }
              }
              else
              {
                set_option(name);
              }
            }
          }
          else if (strncmp(argv[i], "-", 1)==0)
          {
            if (!argv[i][1]) continue;
            char last=argv[i][1];
            for (int j=2; argv[i][j]; j++)
            {
              last=argv[i][j];
              if (lookup.count(argv[i][j-1])==0)
              {
                _errors.push_back(String("undefined short option: -")+argv[i][j-1]);
                continue;
              }
              if (lookup[argv[i][j-1]]=="")
              {
                _errors.push_back(String("ambiguous short option: -")+argv[i][j-1]);
                continue;
              }
              set_option(lookup[argv[i][j-1]]);
            }

            if (lookup.count(last)==0)
            {
              _errors.push_back(String("undefined short option: -")+last);
              continue;
            }
            if (lookup[last]=="")
            {
              _errors.push_back(String("ambiguous short option: -")+last);
              continue;
            }

            if (i+1<argc && _options[lookup[last]]->has_value()){
              set_option(lookup[last], argv[i+1]);
              i++;
            }
            else
            {
              set_option(lookup[last]);
            }
          }
          else
          {
            _others.push_back(argv[i]);
          }
        }

        for (std::map<String, OptionBase*>::iterator p=_options.begin();
             p!=_options.end(); p++)
          if (!p->second->valid())
            _errors.push_back("need option: --"+String(p->first));

        return _errors.size()==0;
      }

      void parse_check(const String &arg)
      {
        if (!_options.count("help"))
            add("help", '?', "print message");
        check(0, parse(arg));
      }

      void parse_check(const Strings &args)
      {
        if (!_options.count("help"))
            add("help", '?', "print message");
        check(args.size(), parse(args));
      }

      void parse_check(int argc, char *argv[]){
        if (!_options.count("help"))
          add("help", '?', "print message");
        check(argc, parse(argc, argv));
      }

      String error() const
      {
        return _errors.size()>0?_errors[0]:"";
      }

      String error_full() const
      {
        std::ostringstream oss;
        for (size_t i=0; i<_errors.size(); i++)
          oss<<_errors[i]<<std::endl;
        return oss.str();
      }

      String usage() const
      {
        std::ostringstream oss;
        oss<<"usage: "<<_progName<<" ";
        for (size_t i=0; i<_ordered.size(); i++)
        {
          if (_ordered[i]->must())
            oss<<_ordered[i]->short_description()<<" ";
        }
    
        oss<<"[options] ... "<<_ftr<<std::endl;
        oss<<"options:"<<std::endl;

        size_t max_width=0;
        for (size_t i=0; i<_ordered.size(); i++)
        {
          max_width=std::max<size_t>(max_width, _ordered[i]->name().length());
        }
        for (size_t i=0; i<_ordered.size(); i++)
        {
          if (_ordered[i]->short_name())
          {
            oss<<"  -"<<_ordered[i]->short_name()<<", ";
          }
          else
          {
            oss<<"      ";
          }

          oss<<"--"<<_ordered[i]->name();
          for (size_t j=_ordered[i]->name().length(); j<max_width+4; j++)
            oss<<' ';
          oss<<_ordered[i]->description()<<std::endl;
        }
        return oss.str();
      }
    private:

      void check(size_t argc, bool ok)
      {
        if ((argc==1 && !ok) || exist("help"))
        {
          std::cerr<<usage();
          exit(0);
        }

        if (!ok){
          std::cerr<<error()<<std::endl<<usage();
          exit(1);
        }
      }

      void set_option(const String &name)
      {
        if (_options.count(name)==0){
          _errors.push_back("undefined option: --"+name);
          return;
        }
        if (!_options[name]->set()){
          _errors.push_back("option needs value: --"+name);
          return;
        }
      }

      void set_option(const String &name, const String &value){
        if (_options.count(name)==0){
          _errors.push_back("undefined option: --"+name);
          return;
        }
        if (!_options[name]->set(value)){
          _errors.push_back("option value is invalid: --"+name+"="+value);
          return;
        }
      }

      class OptionBase
      {
      public:
        virtual ~OptionBase()
        {}
        virtual bool has_value() const=0;
        virtual bool set()=0;
        virtual bool set(const String &value)=0;
        virtual bool has_set() const=0;
        virtual bool valid() const=0;
        virtual bool must() const=0;

        virtual const String &name() const=0;
        virtual char short_name() const=0;
        virtual const String &description() const=0;
        virtual String short_description() const=0;
      };

      class OptionWithoutValue : public OptionBase {
      public:
        OptionWithoutValue(const String &name,
                             char short_name,
                             const String &desc)
          :nam(name), snam(short_name), desc(desc), has(false){
        }
        ~OptionWithoutValue(){}

        bool has_value() const { return false; }

        bool set(){
          has=true;
          return true;
        }

        bool set(const String &){
          return false;
        }

        bool has_set() const {
          return has;
        }

        bool valid() const{
          return true;
        }

        bool must() const{
          return false;
        }

        const String &name() const{
          return nam;
        }

        char short_name() const{
          return snam;
        }

        const String &description() const {
          return desc;
        }

        String short_description() const{
          return "--"+nam;
        }

      private:
        String nam;
        char snam;
        String desc;
        bool has;
      };

      template <class T>
      class OptionWithValue : public OptionBase {
      public:
        OptionWithValue(const String &name,
                          char short_name,
                          bool need,
                          const T &def,
                          const String &desc)
          : nam(name), snam(short_name), need(need), has(false)
          , def(def), actual(def) {
          this->desc=full_description(desc);
        }
        ~OptionWithValue(){}

        const T &get() const {
          return actual;
        }

        bool has_value() const { return true; }

        bool set(){
          return false;
        }

        bool set(const String &value){
          try{
            actual=read(value);
            has=true;
          }
          catch(const std::exception &){
            return false;
          }
          return true;
        }

        bool has_set() const{
          return has;
        }

        bool valid() const{
          if (need && !has) return false;
          return true;
        }

        bool must() const{
          return need;
        }

        const String &name() const{
          return nam;
        }

        char short_name() const{
          return snam;
        }

        const String &description() const {
          return desc;
        }

        String short_description() const{
          return "--"+nam+"="+DETAIL::readableTypename<T>();
        }

      protected:
        String full_description(const String &desc){
          return
            desc+" ("+DETAIL::readableTypename<T>()+
            (need?"":" [="+DETAIL::defaultValue<T>(def)+"]")
            +")";
        }

        virtual T read(const String &s)=0;

        String nam;
        char snam;
        bool need;
        String desc;

        bool has;
        T def;
        T actual;
      };

      template <class T, class F>
      class OptionWithValueWithReader : public OptionWithValue<T> {
      public:
        OptionWithValueWithReader(const String &name,
                                      char short_name,
                                      bool need,
                                      const T def,
                                      const String &desc,
                                      F reader)
          : OptionWithValue<T>(name, short_name, need, def, desc), reader(reader){
        }

      private:
        T read(const String &s){
          return reader(s);
        }

        F reader;
      };

      std::map<String, OptionBase*> _options;
      std::vector<OptionBase*>      _ordered;
      String    _ftr;
      String    _progName;
      Strings   _others;
      Strings   _errors;
    };

} 
