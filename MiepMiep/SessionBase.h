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
		MetaData m_MetaData;
	};


	class SessionBase: public ParentNetwork, public ISession, public EventListener<ISessionListener>, public ITraceable
	{
	public:
		SessionBase(Network& network);
		INetwork& network() const override;
		u32 id() const;
		const MasterSessionData& msd() const;

		bool start();
		const char* name() const override;

		void bufferMsg( const vector<sptr<const NormalSendPacket>>& data, byte channel );
		virtual bool addLink( const sptr<Link>& link );
		virtual void removeLink( Link& link ) = 0;
		bool hasLink( const Link& link ) const;
		void forLink( const Link* exclude, const std::function<void( Link& )>& cb );

        u32 id() const { return m_Id; }

		MM_TO_PTR( SessionBase )

	protected:
		u32  m_Id;
		bool m_Started;
		MasterSessionData m_MasterData;
		vector<wptr<Link>> m_Links;
		vector<sptr<const NormalSendPacket>> m_BufferedPackets[MM_NUM_CHANNELS]; // Pre-fragmented buffered packets.

		friend class Link;
	};
}
