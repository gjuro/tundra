#ifndef incl_Communication_OSIMConnection_h
#define incl_Communication_OSIMConnection_h

#include "Foundation.h"
#include "..\interface.h"
#include <QStringList>
#include "ChatSession.h"

#define OPENSIM_IM_PROTOCOL "OpensimUDP"

namespace OpensimIM
{

	/**
	 *  A connection presentation to Opensim based world server.
	 *  The actual udp connections is already established when connecting to wrold
	 *  this class just encapsultates the IM functionality of that udp connection
	 *  Close method of this class does not close the underlaying udp connections
	 *  but just set this object to logical closed state
	 */
	class Connection : public Communication::ConnectionInterface
	{
		Q_OBJECT
		MODULE_LOGGING_FUNCTIONS
		static const std::string NameStatic() { return "Communication(OpensimIM)"; } // for logging functionality

		static const int GlobalChatChannelID = 0;
		static const int SayDistance = 30;
		static const int ShoutDistance = 100;
		static const int WhisperDistance = 10;
		static enum ChatType { Whisper = 0, Say = 1, Shout = 2, StartTyping = 4, StopTyping = 5, DebugChannel = 6, Region = 7, Owner = 8, Broadcast = 0xFF };
		static enum ChatAudibleLevel { Not = -1, Barely = 0, Fully = 1 };
		static enum ChatSourceType { System = 0, Agent = 1, Object = 2 };

	public:
		Connection(Foundation::Framework* framework);
		virtual ~Connection();

		//! Provides name of the connection
		virtual QString GetName() const;

		//! Connection protocol
		virtual QString GetProtocol() const;

		//! Connection state
		virtual State GetState() const;

		//! Provides server address of this IM server connection
		virtual QString GetServer() const;

		//! Provides textual descriptions about error
		//! If state is not STATE_ERROR then return empty
		virtual QString GetReason() const;

		//! Provides contact list associated with this IM server connection
		virtual Communication::ContactGroupInterface* GetContacts() const;

		//! Provides a list of availble presence status options to set
		virtual QStringList GetAvailablePresenceStatusOptions() const;

		//! Open new chat session with given contact
		//! This is opensim IM chat
		virtual Communication::ChatSessionInterface* OpenChatSession(const Communication::ContactInterface &contact);

		//! Open new chat session to given room
		virtual Communication::ChatSessionInterface* OpenChatSession(const QString &channel);

		//! Send a friend request to target address
		virtual void SendFriendRequest(const QString &target, const QString &message);

		//! Provides all received friend requests with in this connection session
		//! FriendRequest object state must be checked to find out new ones.
		//! If friend request is not answered the server will resend it on next 
		//! connection
		virtual Communication::FriendRequestVector GetFriendRequests() const;

		//! Closes the connection
		virtual void Close();

		bool HandleNetworkEvent(Foundation::EventDataInterface* data);

		bool HandleNetworkStateEvent(Foundation::EventDataInterface* data);

	protected:
		//! Add console commands: 
		virtual void RegisterConsoleCommands();

		virtual void RequestFriendlist();

		//!
		virtual bool HandleOSNEChatFromSimulator(NetInMessage& msg);

		//! Opensim based servers have one global chat channel (id = "0")
		//! We create ChatSession object automatically when connection is established
		virtual void OpenWorldChatSession();

	private:
		Foundation::Framework* framework_;
		QString name_;
		QString protocol_;
		State state_;
		QString server_;
		QString reason_;
		ChatSessionVector public_chat_sessions_;
		ChatSessionVector im_chat_sessions_;
		QString agent_uuid_; 
	public slots:
		void OnWorldChatMessageReceived(const QString& text, const Communication::ChatSessionParticipantInterface& participant);
	};
	typedef std::vector<Connection*> ConnectionVector;

} // end of namespace: OpensimIM

#endif incl_Communication_OSIMConnection_h
