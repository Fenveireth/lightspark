/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef _SECURITY_H
#define _SECURITY_H

#include "compat.h"
#include <string>
#include <vector>
#include <map>
#include <inttypes.h>
#include "swftypes.h"

namespace lightspark
{

class PolicyFile;
class URLPolicyFile;

class SecurityManager
{
public:
	enum SANDBOXTYPE { REMOTE=1, LOCAL_WITH_FILE=2, LOCAL_WITH_NETWORK=4, LOCAL_TRUSTED=8 };
private:
	sem_t mutex;

	const char* sandboxNames[4];
	const char* sandboxTitles[4];

	//Map (by domain) of vectors of pending policy files
	std::multimap<tiny_string, URLPolicyFile*> pendingURLPolicyFiles;
	//Map (by domain) of vectors of loaded policy files
	std::multimap<tiny_string, URLPolicyFile*> loadedURLPolicyFiles;

	//Security sandbox type
	SANDBOXTYPE sandboxType;
	//Use exact domains for player settings
	bool exactSettings;
	//True if exactSettings was already set once
	bool exactSettingsLocked;
public:
	SecurityManager();
	~SecurityManager();

	//Add a policy file located at url, will decide what type of policy file it is.
	PolicyFile* addPolicyFile(const tiny_string& url) { return addPolicyFile(URLInfo(url)); }
	PolicyFile* addPolicyFile(const URLInfo& url);
	//Add an URL policy file located at url
	URLPolicyFile* addURLPolicyFile(const URLInfo& url);
	//Get the URL policy file object (if any) for the URL policy file at url
	URLPolicyFile* getURLPolicyFileByURL(const URLInfo& url);

	void loadPolicyFile(URLPolicyFile* file);
	
	//Set the sandbox type
	void setSandboxType(SANDBOXTYPE type) { sandboxType = type; }
	SANDBOXTYPE getSandboxType() const { return sandboxType; }
	const char* getSandboxName() const { return getSandboxName(sandboxType); }
	const char* getSandboxName(SANDBOXTYPE type) const 
	{
		if(type == REMOTE) return sandboxNames[0];
		else if(type == LOCAL_WITH_FILE) return sandboxNames[1];
		else if(type == LOCAL_WITH_NETWORK) return sandboxNames[2];
		else if(type == LOCAL_TRUSTED) return sandboxNames[3];
		return NULL;
	}
	const char* getSandboxTitle() const { return getSandboxTitle(sandboxType); }
	const char* getSandboxTitle(SANDBOXTYPE type) const
	{
		if(type == REMOTE) return sandboxTitles[0];
		else if(type == LOCAL_WITH_FILE) return sandboxTitles[1];
		else if(type == LOCAL_WITH_NETWORK) return sandboxTitles[2];
		else if(type == LOCAL_TRUSTED) return sandboxTitles[3];
		return NULL;
	}
	//Set exactSettings
	void setExactSettings(bool settings, bool locked=true)
	{ 
		if(!exactSettingsLocked) { 
			exactSettings = settings;
			exactSettingsLocked = locked;
		}
	}
	bool getExactSettings() const { return exactSettings; }
	bool getExactSettingsLocked() const { return exactSettingsLocked; }
	
	//Evaluates whether the current sandbox is in the allowed sandboxes
	bool evaluateSandbox(int allowedSandboxes)
	{ return evaluateSandbox(sandboxType, allowedSandboxes); }
	bool evaluateSandbox(SANDBOXTYPE sandbox, int allowedSandboxes);

	//The possible results for the URL evaluation methods below
	enum EVALUATIONRESULT { ALLOWED, NA_RESTRICT_LOCAL_DIRECTORY,
		NA_REMOTE_SANDBOX, 	NA_LOCAL_SANDBOX, NA_CROSSDOMAIN_POLICY };
	
	//Evaluates an URL by checking allowed sandboxes and checking URL policy files
	EVALUATIONRESULT evaluateURL(const tiny_string& url, bool loadPendingPolicies, 
			int allowedSandboxesRemote, int allowedSandboxesLocal, bool restrictLocalDirectory)
	{
		return evaluateURL(URLInfo(url), loadPendingPolicies,
			allowedSandboxesRemote, allowedSandboxesLocal);
	}
	EVALUATIONRESULT evaluateURL(const URLInfo& url, bool loadPendingPolicies, 
			int allowedSandboxesRemote, int allowedSandboxesLocal, bool restrictLocalDirectory=true);
	
	//Evaluates an URL by checking if the type of URL (local/remote) matches the allowed sandboxes
	EVALUATIONRESULT evaluateSandboxURL(const URLInfo& url,
			int allowedSandboxesRemote, int allowedSandboxesLocal);
	//Evaluates a (local) URL by checking if it points to a subdirectory of the origin
	EVALUATIONRESULT evaluateLocalDirectoryURL(const URLInfo& url);
	//Checks URL policy files
	EVALUATIONRESULT evaluatePoliciesURL(const URLInfo& url, bool loadPendingPolicies);

	//TODO: add evaluateSocketConnection() for SOCKET policy files
};

class PolicySiteControl;
class PolicyAllowAccessFrom;
class PolicyAllowHTTPRequestHeadersFrom;
//TODO: add support for SOCKET policy files
class PolicyFile
{
public:
	enum TYPE { URL, SOCKET };
protected:
	sem_t mutex;

	URLInfo url;
	TYPE type;

	//Is this PolicyFile object valid?
	//Reason for invalidness can be: incorrect URL, download failed (in case of URLPolicyFiles)
	bool valid;
	//Ignore this object?
	//Reason for ignoring can be: master policy file doesn't allow other/any policy files
	bool ignore;
	//Is this file loaded and parsed yet?
	bool loaded;

	PolicySiteControl* siteControl;
	std::vector<PolicyAllowAccessFrom*> allowAccessFrom;
public:
	PolicyFile(URLInfo _url, TYPE _type);
	virtual ~PolicyFile();

	const URLInfo& getURL() const { return url; }
	TYPE getType() const { return type; }

	bool isValid() const { return valid; }
	bool isIgnored() const { return ignore; }
	virtual bool isMaster()=0;
	bool isLoaded() const { return loaded; }
	//Load and parse the policy file
	virtual void load()=0;

	//Get the master policy file controlling this one
	virtual PolicyFile* getMasterPolicyFile()=0;

	const PolicySiteControl* getSiteControl() const { return siteControl ;}
};

class URLPolicyFile : public PolicyFile
{
public:
	enum SUBTYPE { HTTP, HTTPS, FTP };
private:
	URLInfo originalURL;
	SUBTYPE subtype;

	std::vector<PolicyAllowHTTPRequestHeadersFrom*> allowHTTPRequestHeadersFrom;
public:
	URLPolicyFile(const URLInfo& _url);
	~URLPolicyFile();

	const URLInfo& getOriginalURL() const { return originalURL; }
	SUBTYPE getSubtype() const { return subtype; }

	//If strict is true, the policy will be loaded first to see if it isn't redirected
	bool isMaster();
	//Load and parse the policy file
	void load();
	//Get the master policy file controlling this one
	URLPolicyFile* getMasterPolicyFile();

	//Is access to the policy file URL allowed by this policy file?
	bool allowsAccessFrom(const URLInfo& url, const URLInfo& to);
	//Is this request header allowed by this policy file for the given URL?
	bool allowsHTTPRequestHeaderFrom(const URLInfo& u, const URLInfo& to, const tiny_string& header);
};

//Site-wide declarations for master policy file
//Only valid inside master policy files
class PolicySiteControl
{
public:
	enum METAPOLICYTYPE { 
		ALL, //All types of policy files are allowed (default for SOCKET)
		BY_CONTENT_TYPE, //Only policy files served with 'Content-Type: text/x-cross-domain-policy' are allowed (only for HTTP)
		BY_FTP_FILENAME, //Only policy files with 'crossdomain.xml' as filename are allowed (only for FTP)
		MASTER_ONLY, //Only this master policy file is allowed (default for HTTP/HTTPS/FTP)
		NONE, //No policy files are allowed, including this master policy file
		NONE_THIS_RESPONSE //Don't use this policy file, provided as a HTTP header only (TODO: support this type)
	};
private:
	PolicyFile* file;
	METAPOLICYTYPE permittedCrossDomainPolicies; //Required
public:
	PolicySiteControl(PolicyFile* _file, const std::string _permittedCrossDomainPolicies="");
	METAPOLICYTYPE getPermittedCrossDomainPolicies() const { return permittedCrossDomainPolicies; }
};

class PortRange
{
private:
	uint16_t startPort;
	uint16_t endPort;
	bool isRange;
	bool matchAll;
public:
	PortRange(uint16_t _startPort, uint16_t _endPort=0, bool _isRange=false, bool _matchAll=false):
		startPort(_startPort),endPort(_endPort),isRange(_isRange),matchAll(_matchAll){};
	bool matches(uint16_t port)
	{
		if(matchAll)
			return true;
		if(isRange)
		{
			return startPort >= port && endPort <= port;
		}
		else
			return startPort == port;
	}
};

//Permit access by documents from specified domains
class PolicyAllowAccessFrom
{
private:
	PolicyFile* file;
	std::string domain; //Required
	std::vector<PortRange*> toPorts; //Only used for SOCKET policy files, required
	bool secure; //Only used for SOCKET & HTTPS, optional, default: SOCKET=false, HTTPS=true
public:
	PolicyAllowAccessFrom(PolicyFile* _file, const std::string _domain, const std::string _toPorts, bool _secure, bool secureSpecified);
	~PolicyAllowAccessFrom();
	const std::string& getDomain() const { return domain; }
	size_t getToPortsLength() const { return toPorts.size(); }
	PortRange const* getToPort(size_t index) const { return toPorts[index]; }
	bool getSecure() const { return secure; }

	//Does this entry allow a given URL?
	bool allowsAccessFrom(const URLInfo& url) const;
};

//Permit HTTP request header sending (only for HTTP)
class PolicyAllowHTTPRequestHeadersFrom
{
private:
	URLPolicyFile* file;
	std::string domain; //Required
	std::vector<std::string*> headers; //Required
	bool secure; //Only used for HTTPS, optional, default=true
public:
	PolicyAllowHTTPRequestHeadersFrom(URLPolicyFile* _file, const std::string _domain, const std::string _headers, bool _secure, bool secureSpecified);
	~PolicyAllowHTTPRequestHeadersFrom();
	const std::string getDomain() const { return domain; }
	size_t getHeadersLength() const { return headers.size(); }
	std::string const* getHeader(size_t index) const { return headers[index]; }
	bool getSecure() const { return secure; }

	//Does this entry allow a given request header for a given URL?
	bool allowsHTTPRequestHeaderFrom(const URLInfo& url, const tiny_string& header) const;
};

}

#endif
