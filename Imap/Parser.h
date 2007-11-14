/* Copyright (C) 2007 Jan Kundrát <jkt@gentoo.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef IMAP_PARSER_H
#define IMAP_PARSER_H
#include <deque>
#include <memory>
#include <QObject>
#include <QMutex>
#include <QThread>
#include <QSemaphore>
#include <QIODevice>
#include <Imap/Response.h>
#include <Imap/Command.h>
#include <Imap/Exceptions.h>

/**
 * @file
 * A header file defining Parser class and various helpers.
 *
 * @author Jan Kundrát <jkt@gentoo.org>
 */

class QDateTime;
template<class T> class QList;

/** Namespace for IMAP interaction */
namespace Imap {

    /** Flag as defined in RFC3501, section 2.3.2, FIXME */
    class Flag;

    /** Threading algorithm for THREAD command */
    enum ThreadAlgorithm {
        THREAD_ORDEREDSUBJECT /**< ORDEREDSUBJECT algorithm */,
        THREAD_REFERENCES /**< REFERENCES algorithm */
    };

    class Authenticator {};
    class Message;

    /** Class specifying a set of messagess to access */
    class Sequence {
    public:
        /** Converts sequence to string suitable for sending over the wire */
        QString toString() const {return "";};
    };

    /** A handle identifying a command sent to the server */
    typedef QString CommandHandle;

    class Parser; // will be defined later

    /** Helper thread for Parser that deals with actual I/O */
    class WorkerThread : public QThread {
        Q_OBJECT

        virtual void run();

        /** Prevent copying */
        WorkerThread( const WorkerThread& );
        /** Prevent copying */
        WorkerThread& operator=( const WorkerThread& );

        /** Reference to our parser */
        Parser* _parser;

    public:
        WorkerThread( Parser * const parser ) : _parser( parser ) {};
    };

    /** Class that does all IMAP parsing */
    class Parser : public QObject {
        Q_OBJECT

        friend class WorkerThread;

    public:
        /** Constructor. Takes an QIODevice instance as a parameter. */
        Parser( QObject* parent, std::auto_ptr<QIODevice> socket );

        /** Destructor */
        ~Parser();

    public slots:

        /** CAPABILITY, RFC 3501 section 6.1.1 */
        CommandHandle capability();

        /** NOOP, RFC 3501 section 6.1.2 */
        CommandHandle noop();

        /** LOGOUT, RFC3501 section 6.1.3 */
        CommandHandle logout();


        /** STARTTLS, RFC3051 section 6.2.1 */
        CommandHandle startTls();

        /** AUTHENTICATE, RFC3501 section 6.2.2 */
        CommandHandle authenticate( /* FIXME: parameter */ );

        /** LOGIN, RFC3501 section 6.2.3 */
        CommandHandle login( const QString& user, const QString& pass );


        /** SELECT, RFC3501 section 6.3.1 */
        CommandHandle select( const QString& mailbox );

        /** EXAMINE, RFC3501 section 6.3.2 */
        CommandHandle examine( const QString& mailbox );

        /** CREATE, RFC3501 section 6.3.3 */
        CommandHandle create( const QString& mailbox );

        /** DELETE, RFC3501 section 6.3.4 */
        CommandHandle deleteMailbox( const QString& mailbox );

        /** RENAME, RFC3501 section 6.3.5 */
        CommandHandle rename( const QString& oldName, const QString& newName );

        /** SUBSCRIBE, RFC3501 section 6.3.6 */
        CommandHandle subscribe( const QString& mailbox );
        
        /** UNSUBSCRIBE, RFC3501 section 6.3.7 */
        CommandHandle unSubscribe( const QString& mailbox );

        /** LIST, RFC3501 section 6.3.8 */
        CommandHandle list( const QString& reference, const QString& mailbox );
        
        /** LSUB, RFC3501 section 6.3.9 */
        CommandHandle lSub( const QString& reference, const QString& mailbox );

        /** STATUS, RFC3501 section 6.3.10 */
        CommandHandle status( const QString& mailbox, const QStringList& fields );
        
        /** APPEND, RFC3501 section 6.3.11 */
        CommandHandle append( const QString& mailbox, const QString& message, const QStringList& flags = QStringList(), const QDateTime& timestamp = QDateTime() );


        /** CHECK, RFC3501 sect 6.4.1 */
        CommandHandle check();

        /** CLOSE, RFC3501 sect 6.4.2 */
        CommandHandle close();
        
        /** EXPUNGE, RFC3501 sect 6.4.3 */
        CommandHandle expunge();

        /** SEARCH, RFC3501 sect 6.4.4 */
        CommandHandle search( const QStringList& criteria, const QString& charset = QString::null ) {
            return _searchHelper( "SEARCH", criteria, charset );
        };

        /** FETCH, RFC3501 sect 6.4.5 */
        CommandHandle fetch( const Sequence& seq, const QStringList& items );

        /** STORE, RFC3501 sect 6.4.6 */
        CommandHandle store( const Sequence& seq, const QString& item, const QString& value );

        /** COPY, RFC3501 sect 6.4.7 */
        CommandHandle copy( const Sequence& seq, const QString& mailbox );

        /** UID command (FETCH), RFC3501 sect 6.4.8 */
        CommandHandle uidFetch( const Sequence& seq, const QStringList& items );

        /** UID command (SEARCH), RFC3501 sect 6.4.8 */
        CommandHandle uidSearch( const QStringList& criteria, const QString& charset ) {
            return _searchHelper( "UID SEARCH", criteria, charset );
        };


        /** X<atom>, RFC3501 sect 6.5.1 */
        CommandHandle xAtom( const Commands::Command& commands );


        /** UNSELECT, RFC3691 */
        CommandHandle unSelect();
        
        /** IDLE, RFC2177 */
        CommandHandle idle();


#if 0
        /** SORT, draft-ietf-imapext-sort-19, section 3 */
        CommandHandle sort( /*const SortAlgorithm& algo,*/ const QString& charset, const QStringList& criteria );
        /** UID SORT, draft-ietf-imapext-sort-19, section 3 */
        CommandHandle uidSort( /*const SortAlgorithm& algo,*/ const QString charset, const QStringList& criteria );
        /** THREAD, draft-ietf-imapext-sort-19, section 3 */
        CommandHandle thread( const ThreadAlgorithm& algo, const QString charset, const QStringList& criteria );
#endif

    private slots:

        /** Socket told us that we can read data */
        void socketReadyRead();

        /** Socket got disconnected */
        void socketDisconected();

    signals:
        /** Socket got disconnected */
        void disconnected();

        /** New response received */
        void responseReceived();

    private:
        /** Private copy constructor */
        Parser( const Parser& );
        /** Private assignment operator */
        Parser& operator=( const Parser& );

        /** Queue command for execution.*/
        CommandHandle queueCommand( Commands::Command command );

        /** Shortcut function; works exactly same as above mentioned queueCommand() */
        CommandHandle queueCommand( const Commands::TokenType kind, const QString& text ) {
            return queueCommand( Commands::Command() << Commands::PartOfCommand( kind, text ) );
        };

        /** Helper for search() and uidSearch() */
        CommandHandle _searchHelper( const QString& command, const QStringList& criteria, const QString& charset = QString::null );

        /** Generate tag for next command */
        QString generateTag();

        /** Wait for a command continuation request being sent by the server */
        void waitForContinuationRequest();

        /** Execute first queued command, ie. send it to the server */
        bool executeIfPossible();
        /** Execute passed command right now */
        bool executeACommand( const Commands::Command& cmd );

        /** Process a line from IMAP server */
        void processLine( const QByteArray& line );

        /** Parse line for untagged reply */
        void parseUntagged( const QByteArray& line );

        /** Parse line for tagged reply */
        void parseTagged( const QByteArray& line );

        /** Constructs ResponseCode instance from a pair of iterators.
         *
         * This function modifies its first argument so it points to the
         * beginning of non-response-code data (which might be 'end').
         */
        QList<QByteArray> _parseResponseCode( QList<QByteArray>::const_iterator& begin, const QList<QByteArray>::const_iterator& end ) const;

        /** Add parsed response to the internal queue, emit notification signal */
        void queueResponse( const Response& resp );

        /** Connection to the IMAP server */
        std::auto_ptr<QIODevice> _socket;

        /** Keeps track of the last-used command tag */
        unsigned int _lastTagUsed;

        /** Mutex for synchronizing access to our queues */
        QMutex _cmdMutex, _respMutex;

        /** Queue storing commands that are about to be executed */
        std::deque<Commands::Command> _cmdQueue;

        /** Queue storing parsed replies from the IMAP server */
        std::deque<Response> _respQueue;

        /** Worker thread instance */
        WorkerThread _workerThread;

        /** Used for throttling worker thread's activity */
        QSemaphore _workerSemaphore;

        /** Ask worker thread to stop ASAP */
        bool _workerStop;

        /** Mutex guarding _workerStop */
        QMutex _workerStopMutex;

    };

}
#endif /* IMAP_PARSER_H */
