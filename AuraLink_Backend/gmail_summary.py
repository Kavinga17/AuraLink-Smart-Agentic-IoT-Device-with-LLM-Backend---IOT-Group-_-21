"""
AuraLink Gmail Summary -> MQTT Publisher

Features:
1. Authenticate with Gmail
2. Fetch unread emails from the last 30 minutes
3. Summarize email content using Gemini AI
4. Publish summary to MQTT topic: kp_home/email_summary
5. Display latest summary for 5 minutes only on ESP32 LCD
"""

import os
import pickle
import base64
import datetime
import time
import signal
import sys
import paho.mqtt.client as mqtt
from dateutil import parser
from dateutil.relativedelta import relativedelta
from google.auth.transport.requests import Request
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build
from google.oauth2.credentials import Credentials
from datetime import timezone
from summarizer import simple_summarize
import gemini_client
from gmail_helpers import get_any_unread_emails

# Gmail API scope
SCOPES = ['https://www.googleapis.com/auth/gmail.readonly']

# Gemini API
GEMINI_API_KEY = "AIzaSyD-Do_2m40IG5PR6AKDh5qiPc7mcz0MiZc"
gemini_client.configure_gemini(GEMINI_API_KEY)

# MQTT configuration
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC = "kp_home/email_summary"
mqtt_client = mqtt.Client()

# Track processed emails
processed_emails = set()
last_summary_time = 0
summary_display_duration = 5 * 60  # 5 minutes in seconds

def authenticate_gmail():
    """Authenticate with Gmail API and return service"""
    creds = None
    token_path = 'token.pickle'
    if os.path.exists(token_path):
        with open(token_path, 'rb') as token:
            creds = pickle.load(token)
    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            flow = InstalledAppFlow.from_client_secrets_file('credentials.json', SCOPES)
            creds = flow.run_local_server(port=0)
        with open(token_path, 'wb') as token:
            pickle.dump(creds, token)
    return build('gmail', 'v1', credentials=creds)

def get_email_content(service, msg_id):
    """Get email content without marking as read"""
    message = service.users().messages().get(userId='me', id=msg_id, format='full').execute()
    headers = message['payload']['headers']
    subject = next((h['value'] for h in headers if h['name'].lower() == 'subject'), 'No Subject')
    sender = next((h['value'] for h in headers if h['name'].lower() == 'from'), 'Unknown Sender')
    date_str = next((h['value'] for h in headers if h['name'].lower() == 'date'), None)
    received_date = parser.parse(date_str) if date_str else None

    def get_message_parts(payload):
        if 'parts' in payload:
            for part in payload['parts']:
                body_data = get_message_parts(part)
                if body_data:
                    return body_data
        elif payload.get('body', {}).get('data'):
            return base64.urlsafe_b64decode(payload['body']['data']).decode('utf-8')
        return None

    body = get_message_parts(message['payload']) or "No content found"
    return {'id': msg_id, 'subject': subject, 'sender': sender, 'received_date': received_date, 'body': body}

def summarize_with_gemini(text):
    """Summarize text using Gemini AI or fallback"""
    summary, success = gemini_client.summarize(text, 80)
    if success and summary:
        return summary
    return simple_summarize(text, 80)

def check_emails(service):
    """Check for unread emails in last 30 minutes"""
    global last_summary_time
    emails = get_any_unread_emails(service)
    if not emails:
        return None
    latest_email = emails[0]
    if latest_email['id'] in processed_emails:
        return None

    email_data = get_email_content(service, latest_email['id'])
    now = datetime.datetime.now(timezone.utc)
    thirty_mins_ago = now - relativedelta(minutes=30)
    received_date_utc = None
    if email_data['received_date']:
        received_date_utc = (email_data['received_date'].astimezone(timezone.utc)
                             if email_data['received_date'].tzinfo else
                             email_data['received_date'].replace(tzinfo=timezone.utc))

    if received_date_utc and received_date_utc >= thirty_mins_ago:
        summary = summarize_with_gemini(email_data['body'])
        processed_emails.add(latest_email['id'])
        last_summary_time = time.time()
        print(f"Publishing summary to MQTT: {summary}")
        mqtt_client.publish(MQTT_TOPIC, summary)
        return summary
    return None

def signal_handler(sig, frame):
    print("Exiting AuraLink Gmail Summary.")
    sys.exit(0)

def main():
    signal.signal(signal.SIGINT, signal_handler)
    print("Authenticating with Gmail...")
    gmail_service = authenticate_gmail()
    print("Connecting to MQTT broker...")
    mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
    mqtt_client.loop_start()
    print("AuraLink Gmail Summary running. Ctrl+C to exit.")

    while True:
        summary = check_emails(gmail_service)
        if summary:
            print(f"New Email Summary: {summary}")
        time.sleep(60)

if __name__ == "__main__":
    main()
