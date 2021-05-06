#pragma once

typedef class _aWebServiceScheme* aWebServiceScheme;
typedef class _aHttpService* aHttpService;
typedef class _aDataElement* aDataElement;

struct _aWebServiceScheme {
   std::string scheme;

   virtual bool Invoke(aDataElement request, aDataElement response);
};

struct _aHttpService {
   virtual bool Invoke(aDataElement request, aDataElement response);
};

struct _aHttpServiceScheme : _aWebServiceScheme {
   std::string scheme;

   virtual AddService(aHttpService service);
   virtual bool Invoke(aDataElement request, aDataElement response);
};

struct WebServicesManager {

   virtual AddService(aWebServiceScheme service);
   virtual bool Invoke(aDataElement request, aDataElement response);
};
