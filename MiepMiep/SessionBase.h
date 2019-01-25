#pragma once


#include "Memory.h"
#include "ParentNetwork.h"
#include "MiepMiep.h"
#include "NetworkEvents.h"
#include "Threading.h"


namespace MiepMiep
{
	class  Link;
	class  AddressList;
	struct NormalSendPacket;

	// ----- MasterSession client/server shared per link data  ------------------------------------------------------------------

	// TODO change for variable group instead!
	struct MasterSessionData
	{
		bool  m_IsP2p;
		bool  m_UsedMatchmaker;
		bool  m_IsPrivate;
		float m_Rating;
		u32	  m_MaxClients;
		bool  m_CanJoinAfterStart;
		std::string m_Name;
		std::string m_Type;
		std::string m_Password;
	};


	class SessionBase: public ParentNetwork, public ISession, public EventListener<ISessionListener>, public ITraceable
	{
	public:
		SessionBase(Network& network);
		MM_TSC INetwork& network() const override;
        MM_TSC u32 id() const;
		MM_TSC rmutex& dataMutex() { return m_DataMutex; }
		MM_TSC const MasterSessionData& msd() const; // TODO retrieving members of this is not TS 

		MM_TS bool start();
		MM_TS const char* name() const override;

		MM_TS void bufferMsg( const vector<sptr<const NormalSendPacket>>& data, byte channel );
		MM_TS virtual bool addLink( const sptr<Link>& link);
		MM_TS virtual void removeLink( Link& link ) = 0;
		MM_TS bool hasLink( const Link& link ) const;
		MM_TS void forLink( const Link* exclude, const std::function<void( Link& )>& cb );

		MM_TO_PTR( SessionBase )

	protected:
		mutable rmutex m_DataMutex;
		u32  m_Id;
		bool m_Started;
		MasterSessionData m_MasterData;
		vector<wptr<Link>> m_Links;
		vector<sptr<const NormalSendPacket>> m_BufferedPackets[MM_NUM_CHANNELS]; // Pre-fragmented buffered packets.

		friend class Link;
	};
}
