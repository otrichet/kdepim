#include <qstring.h>

#ifndef KAB_TELEPHONE_H
#define KAB_TELEPHONE_H

namespace KAB
{

class Telephone
{
  public:
    
    Telephone()
    {
      // Empty.
    }
    
    Telephone(const Telephone & t)
      : countryCode_  (t.countryCode_ ),
        areaCode_     (t.areaCode_    ),
        number_       (t.number_      ),
        extension_    (t.extension_   )
    {
      // Empty.
    }
 
    ~Telephone()
    {
      // Empty.
    }
    
    Telephone & operator = (const Telephone & t)
    {
      if (this == &t) return *this;
      
      countryCode_  = t.countryCode_;
      areaCode_     = t.areaCode_;
      number_       = t.number_;
      extension_    = t.extension_;

      return *this; 
    }
    
    bool operator == (const Telephone & t) const
    {
      return (
        (countryCode_  == t.countryCode_) &&
        (areaCode_     == t.areaCode_   ) &&
        (number_       == t.number_     ) &&
        (extension_    == t.extension_  ));
    }
    
    QString countryCode()   const { return countryCode_;  }
    QString areaCode()      const { return areaCode_;     }
    QString number()        const { return number_;       }
    QString extension()     const { return extension_;    }

    void  setCountryCode  (const QString & s) { countryCode_  = s; }
    void  setAreaCode     (const QString & s) { areaCode_     = s; }
    void  setNumber       (const QString & s) { number_       = s; }
    void  setExtension    (const QString & s) { extension_    = s; }
    
    virtual void save(QDataStream & str);
    virtual void load(QDataStream & str);

  private:
    
    QString countryCode_;
    QString areaCode_;
    QString number_;
    QString extension_;
};

  void
Telephone::save(QDataStream & str)
{
  str << countryCode_ << areaCode_ << number_ << extension_;
}

  void
Telephone::load(QDataStream & str)
{
  str >> countryCode_ >> areaCode_ >> number_ >> extension_;
}

} // End namespace KAB

#endif

