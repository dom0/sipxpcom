#include "nsSIP.h"
#include "nsStringAPI.h"



NS_IMPL_ISUPPORTS1(nsSIP, nsISIP)


nsSIP::nsSIP() : mObservers(nsnull), proxy(nsnull) {
  port = 0;
}

nsSIP::~nsSIP() {
  FlushObservers();
}


/* void init (in long port); */
NS_IMETHODIMP nsSIP::Init(PRInt32 _port)
{

  if (port!=0){
    Destroy();
  }
  if (_port<1024)
    return NS_ERROR_ILLEGAL_VALUE;
  port = _port;
  sipregister((int)port); 
  return NS_OK;
}


/* void destroy (); */
NS_IMETHODIMP nsSIP::Destroy() {
  if (port==0)
    return NS_OK;

  CallObservers("DESTROY");
  FlushObservers();
  sipderegister();
  port = 0;
  return NS_OK;
}


/* void call (in AString URI); */
NS_IMETHODIMP nsSIP::Call(const char* URI) {
  sipmakecall((char*)URI);
  return NS_OK;
}


/* void hangup (); */
NS_IMETHODIMP nsSIP::Hangup() {
  siphangup();
  return NS_OK;
}


/* void addObserver (in nsSipStateObserver cbk); */
NS_IMETHODIMP nsSIP::AddObserver(nsSipStateObserver *cbk)
{
  NS_ENSURE_ARG_POINTER(cbk);

  if (!mObservers) {
    mObservers = do_CreateInstance(NS_ARRAY_CONTRACTID);
    proxy = do_CreateInstance(NS_ARRAY_CONTRACTID);
    NS_ENSURE_STATE(mObservers);
    NS_ENSURE_STATE(proxy);
  }

  /* TEST IF cbk OBSERVER IS ALREADY REGISTERED */
  PRUint32 count = 0;
  PRIntn i;
  mObservers->GetLength(&count);
  nsCOMPtr<nsSipStateObserver> pCallback;
  for (i = 0; i < count; ++i) {
    (nsIArray*)mObservers->QueryElementAt(i, NS_GET_IID(nsSipStateObserver), (void**)&pCallback);
    if (pCallback == cbk){
      printf("OBSERVER ALREADY REGISTERED\n");
      return NS_OK;
    }
  }

  NS_IF_ADDREF(cbk);
  mObservers->AppendElement(cbk, PR_FALSE);
  printf("ADDED OBSERVER AT ADDR: %p \n", cbk);
  /* PROXY */
  nsCOMPtr<nsSipStateObserver> pCbk;
  getProxyForObserver(cbk, &pCbk);
  NS_IF_ADDREF(pCbk);
  proxy->AppendElement(pCbk, PR_FALSE);
  //SYNC OBSERVERS ARRAY ON pjsip BRIDGE
  SyncObservers((nsIArray*)proxy);
  return NS_OK;
}


/* void removeObserver (in nsSipStateObserver cbk); */
NS_IMETHODIMP nsSIP::RemoveObserver(nsSipStateObserver *cbk)
{
  NS_ENSURE_ARG_POINTER(cbk);

  if (!mObservers){
    printf("NO SUCH OBSERVER\n");
    return NS_OK;
  }

  PRUint32 count = 0;
  mObservers->GetLength(&count);
  if (count <= 0){
    printf("NO SUCH OBSERVER\n");
    return NS_OK;
  }

  PRIntn i;
  nsCOMPtr<nsSipStateObserver> pCallback;
  nsCOMPtr<nsSipStateObserver> _pCallback;

  for (i = 0; i < count; ++i) {
    (nsIArray*)mObservers->QueryElementAt(i, NS_GET_IID(nsSipStateObserver), (void**)&pCallback);
    (nsIArray*)proxy->QueryElementAt(i, NS_GET_IID(nsSipStateObserver), (void**)&_pCallback);
    if (pCallback == cbk){
      mObservers->RemoveElementAt(i);
      proxy->RemoveElementAt(i);
      printf("REMOVED OBSERVER AT ADDR: %p - %p\n", cbk, (nsSipStateObserver*)pCallback);
      //SYNC OBSERVERS ARRAY ON pjsip BRIDGE
      SyncObservers((nsIArray*)proxy);

      NS_RELEASE(_pCallback);
      NS_RELEASE(pCallback);
      return NS_OK;
    }
  }

  printf("NO SUCH OBSERVER\n");
  return NS_OK;
}



void nsSIP::FlushObservers(){
  if (!mObservers)
      return;

  PRUint32 count = 0;
  mObservers->GetLength(&count);

  PRIntn i;
  nsCOMPtr<nsSipStateObserver> pCallback;
  nsCOMPtr<nsSipStateObserver> _pCallback;

  for (i = 0; i < count; ++i) {
    (nsIArray*)mObservers->QueryElementAt(i, NS_GET_IID(nsSipStateObserver), (void**)&pCallback);
    (nsIArray*)proxy->QueryElementAt(i, NS_GET_IID(nsSipStateObserver), (void**)&_pCallback);
    mObservers->RemoveElementAt(i);
    proxy->RemoveElementAt(i);

    NS_RELEASE(pCallback);
    NS_RELEASE(_pCallback);
  }

  printf("FLUSHED ALL OBSERVERS\n");
  NS_RELEASE(mObservers);
  NS_RELEASE(proxy);
  SyncObservers(NULL);
  return;
}


/* void clearObservers (); */
NS_IMETHODIMP nsSIP::ClearObservers()
{
  FlushObservers();
  return NS_OK;
}


void nsSIP::getProxyForObserver(nsCOMPtr<nsSipStateObserver> cbk, nsCOMPtr<nsSipStateObserver> *pCbk){
  nsresult rv = NS_OK;
  nsCOMPtr<nsIProxyObjectManager> pIProxyObjectManager(do_GetService("@mozilla.org/xpcomproxy;1", &rv));

  rv = pIProxyObjectManager->GetProxyForObject(
    NS_PROXY_TO_MAIN_THREAD,
    nsSipStateObserver::GetIID(),
    cbk,
    NS_PROXY_SYNC | NS_PROXY_ALWAYS,
    (void**)pCbk
  );
}


void nsSIP::CallObservers(const char* status)
{
  if (!mObservers)
      return;

  PRUint32 count = 0;
  mObservers->GetLength(&count);
  if (count <= 0)
      return;

  PRIntn i;
  nsCOMPtr<nsSipStateObserver> _pCallback;
  for (i = 0; i < count; ++i) {
    (nsIArray*)mObservers->QueryElementAt(i, NS_GET_IID(nsSipStateObserver), (void**)&_pCallback);
    _pCallback->OnStatusChange(status);
  }

  return;
}




/* void setringtone (in string file); */
NS_IMETHODIMP nsSIP::Setringtone(const char *file)
{
  if (port==0) return NS_OK;
  setringtone((char*)file);
  return NS_OK;
}

/* void playring (); */
NS_IMETHODIMP nsSIP::Playringtone()
{
  if (port==0) return NS_OK;
  playring();
  return NS_OK;
}

/* void stopring (); */
NS_IMETHODIMP nsSIP::Stopringtone()
{
  if (port==0) return NS_OK;
  stopring();
  return NS_OK;
}

/* void playdtmftone (in char tone); */
NS_IMETHODIMP nsSIP::Playdtmftone(const char tone)
{
  if (port==0) return NS_OK;
  playtone((char)tone);
  return NS_OK;
}
