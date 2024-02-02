// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Code generated by aliasgen. DO NOT EDIT.

// Package pubsub aliases all exported identifiers in package
// "cloud.google.com/go/pubsub/apiv1/pubsubpb".
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb.
// Please read https://github.com/googleapis/google-cloud-go/blob/main/migration.md
// for more details.
package pubsub

import (
	src "cloud.google.com/go/pubsub/apiv1/pubsubpb"
	grpc "google.golang.org/grpc"
)

// Deprecated: Please use consts in: cloud.google.com/go/pubsub/apiv1/pubsubpb
const (
	BigQueryConfig_ACTIVE              = src.BigQueryConfig_ACTIVE
	BigQueryConfig_NOT_FOUND           = src.BigQueryConfig_NOT_FOUND
	BigQueryConfig_PERMISSION_DENIED   = src.BigQueryConfig_PERMISSION_DENIED
	BigQueryConfig_SCHEMA_MISMATCH     = src.BigQueryConfig_SCHEMA_MISMATCH
	BigQueryConfig_STATE_UNSPECIFIED   = src.BigQueryConfig_STATE_UNSPECIFIED
	Encoding_BINARY                    = src.Encoding_BINARY
	Encoding_ENCODING_UNSPECIFIED      = src.Encoding_ENCODING_UNSPECIFIED
	Encoding_JSON                      = src.Encoding_JSON
	SchemaView_BASIC                   = src.SchemaView_BASIC
	SchemaView_FULL                    = src.SchemaView_FULL
	SchemaView_SCHEMA_VIEW_UNSPECIFIED = src.SchemaView_SCHEMA_VIEW_UNSPECIFIED
	Schema_AVRO                        = src.Schema_AVRO
	Schema_PROTOCOL_BUFFER             = src.Schema_PROTOCOL_BUFFER
	Schema_TYPE_UNSPECIFIED            = src.Schema_TYPE_UNSPECIFIED
	Subscription_ACTIVE                = src.Subscription_ACTIVE
	Subscription_RESOURCE_ERROR        = src.Subscription_RESOURCE_ERROR
	Subscription_STATE_UNSPECIFIED     = src.Subscription_STATE_UNSPECIFIED
)

// Deprecated: Please use vars in: cloud.google.com/go/pubsub/apiv1/pubsubpb
var (
	BigQueryConfig_State_name          = src.BigQueryConfig_State_name
	BigQueryConfig_State_value         = src.BigQueryConfig_State_value
	Encoding_name                      = src.Encoding_name
	Encoding_value                     = src.Encoding_value
	File_google_pubsub_v1_pubsub_proto = src.File_google_pubsub_v1_pubsub_proto
	File_google_pubsub_v1_schema_proto = src.File_google_pubsub_v1_schema_proto
	SchemaView_name                    = src.SchemaView_name
	SchemaView_value                   = src.SchemaView_value
	Schema_Type_name                   = src.Schema_Type_name
	Schema_Type_value                  = src.Schema_Type_value
	Subscription_State_name            = src.Subscription_State_name
	Subscription_State_value           = src.Subscription_State_value
)

// Request for the Acknowledge method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type AcknowledgeRequest = src.AcknowledgeRequest

// Configuration for a BigQuery subscription.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type BigQueryConfig = src.BigQueryConfig

// Possible states for a BigQuery subscription.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type BigQueryConfig_State = src.BigQueryConfig_State

// Request for the CreateSchema method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type CreateSchemaRequest = src.CreateSchemaRequest

// Request for the `CreateSnapshot` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type CreateSnapshotRequest = src.CreateSnapshotRequest

// Dead lettering is done on a best effort basis. The same message might be
// dead lettered multiple times. If validation on any of the fields fails at
// subscription creation/updation, the create/update subscription request will
// fail.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type DeadLetterPolicy = src.DeadLetterPolicy

// Request for the `DeleteSchema` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type DeleteSchemaRequest = src.DeleteSchemaRequest

// Request for the `DeleteSnapshot` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type DeleteSnapshotRequest = src.DeleteSnapshotRequest

// Request for the DeleteSubscription method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type DeleteSubscriptionRequest = src.DeleteSubscriptionRequest

// Request for the `DeleteTopic` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type DeleteTopicRequest = src.DeleteTopicRequest

// Request for the DetachSubscription method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type DetachSubscriptionRequest = src.DetachSubscriptionRequest

// Response for the DetachSubscription method. Reserved for future use.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type DetachSubscriptionResponse = src.DetachSubscriptionResponse

// Possible encoding types for messages.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type Encoding = src.Encoding

// A policy that specifies the conditions for resource expiration (i.e.,
// automatic resource deletion).
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ExpirationPolicy = src.ExpirationPolicy

// Request for the GetSchema method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type GetSchemaRequest = src.GetSchemaRequest

// Request for the GetSnapshot method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type GetSnapshotRequest = src.GetSnapshotRequest

// Request for the GetSubscription method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type GetSubscriptionRequest = src.GetSubscriptionRequest

// Request for the GetTopic method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type GetTopicRequest = src.GetTopicRequest

// Request for the `ListSchemas` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ListSchemasRequest = src.ListSchemasRequest

// Response for the `ListSchemas` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ListSchemasResponse = src.ListSchemasResponse

// Request for the `ListSnapshots` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ListSnapshotsRequest = src.ListSnapshotsRequest

// Response for the `ListSnapshots` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ListSnapshotsResponse = src.ListSnapshotsResponse

// Request for the `ListSubscriptions` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ListSubscriptionsRequest = src.ListSubscriptionsRequest

// Response for the `ListSubscriptions` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ListSubscriptionsResponse = src.ListSubscriptionsResponse

// Request for the `ListTopicSnapshots` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ListTopicSnapshotsRequest = src.ListTopicSnapshotsRequest

// Response for the `ListTopicSnapshots` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ListTopicSnapshotsResponse = src.ListTopicSnapshotsResponse

// Request for the `ListTopicSubscriptions` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ListTopicSubscriptionsRequest = src.ListTopicSubscriptionsRequest

// Response for the `ListTopicSubscriptions` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ListTopicSubscriptionsResponse = src.ListTopicSubscriptionsResponse

// Request for the `ListTopics` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ListTopicsRequest = src.ListTopicsRequest

// Response for the `ListTopics` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ListTopicsResponse = src.ListTopicsResponse

// A policy constraining the storage of messages published to the topic.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type MessageStoragePolicy = src.MessageStoragePolicy

// Request for the ModifyAckDeadline method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ModifyAckDeadlineRequest = src.ModifyAckDeadlineRequest

// Request for the ModifyPushConfig method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ModifyPushConfigRequest = src.ModifyPushConfigRequest

// Request for the Publish method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type PublishRequest = src.PublishRequest

// Response for the `Publish` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type PublishResponse = src.PublishResponse

// PublisherClient is the client API for Publisher service. For semantics
// around ctx use and closing/ending streaming RPCs, please refer to
// https://godoc.org/google.golang.org/grpc#ClientConn.NewStream.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type PublisherClient = src.PublisherClient

// PublisherServer is the server API for Publisher service.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type PublisherServer = src.PublisherServer

// A message that is published by publishers and consumed by subscribers. The
// message must contain either a non-empty data field or at least one
// attribute. Note that client libraries represent this object differently
// depending on the language. See the corresponding [client library
// documentation](https://cloud.google.com/pubsub/docs/reference/libraries) for
// more information. See [quotas and limits]
// (https://cloud.google.com/pubsub/quotas) for more information about message
// limits.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type PubsubMessage = src.PubsubMessage

// Request for the `Pull` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type PullRequest = src.PullRequest

// Response for the `Pull` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type PullResponse = src.PullResponse

// Configuration for a push delivery endpoint.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type PushConfig = src.PushConfig

// Contains information needed for generating an [OpenID Connect
// token](https://developers.google.com/identity/protocols/OpenIDConnect).
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type PushConfig_OidcToken = src.PushConfig_OidcToken
type PushConfig_OidcToken_ = src.PushConfig_OidcToken_

// A message and its corresponding acknowledgment ID.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ReceivedMessage = src.ReceivedMessage

// A policy that specifies how Cloud Pub/Sub retries message delivery. Retry
// delay will be exponential based on provided minimum and maximum backoffs.
// https://en.wikipedia.org/wiki/Exponential_backoff. RetryPolicy will be
// triggered on NACKs or acknowledgement deadline exceeded events for a given
// message. Retry Policy is implemented on a best effort basis. At times, the
// delay between consecutive deliveries may not match the configuration. That
// is, delay can be more or less than configured backoff.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type RetryPolicy = src.RetryPolicy

// A schema resource.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type Schema = src.Schema

// SchemaServiceClient is the client API for SchemaService service. For
// semantics around ctx use and closing/ending streaming RPCs, please refer to
// https://godoc.org/google.golang.org/grpc#ClientConn.NewStream.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type SchemaServiceClient = src.SchemaServiceClient

// SchemaServiceServer is the server API for SchemaService service.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type SchemaServiceServer = src.SchemaServiceServer

// Settings for validating messages published against a schema.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type SchemaSettings = src.SchemaSettings

// View of Schema object fields to be returned by GetSchema and ListSchemas.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type SchemaView = src.SchemaView

// Possible schema definition types.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type Schema_Type = src.Schema_Type

// Request for the `Seek` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type SeekRequest = src.SeekRequest
type SeekRequest_Snapshot = src.SeekRequest_Snapshot
type SeekRequest_Time = src.SeekRequest_Time

// Response for the `Seek` method (this response is empty).
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type SeekResponse = src.SeekResponse

// A snapshot resource. Snapshots are used in
// [Seek](https://cloud.google.com/pubsub/docs/replay-overview) operations,
// which allow you to manage message acknowledgments in bulk. That is, you can
// set the acknowledgment state of messages in an existing subscription to the
// state captured by a snapshot.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type Snapshot = src.Snapshot

// Request for the `StreamingPull` streaming RPC method. This request is used
// to establish the initial stream as well as to stream acknowledgements and
// ack deadline modifications from the client to the server.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type StreamingPullRequest = src.StreamingPullRequest

// Response for the `StreamingPull` method. This response is used to stream
// messages from the server to the client.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type StreamingPullResponse = src.StreamingPullResponse

// Acknowledgement IDs sent in one or more previous requests to acknowledge a
// previously received message.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type StreamingPullResponse_AcknowledgeConfirmation = src.StreamingPullResponse_AcknowledgeConfirmation

// Acknowledgement IDs sent in one or more previous requests to modify the
// deadline for a specific message.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type StreamingPullResponse_ModifyAckDeadlineConfirmation = src.StreamingPullResponse_ModifyAckDeadlineConfirmation

// Subscription properties sent as part of the response.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type StreamingPullResponse_SubscriptionProperties = src.StreamingPullResponse_SubscriptionProperties

// SubscriberClient is the client API for Subscriber service. For semantics
// around ctx use and closing/ending streaming RPCs, please refer to
// https://godoc.org/google.golang.org/grpc#ClientConn.NewStream.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type SubscriberClient = src.SubscriberClient

// SubscriberServer is the server API for Subscriber service.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type SubscriberServer = src.SubscriberServer
type Subscriber_StreamingPullClient = src.Subscriber_StreamingPullClient
type Subscriber_StreamingPullServer = src.Subscriber_StreamingPullServer

// A subscription resource.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type Subscription = src.Subscription

// Possible states for a subscription.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type Subscription_State = src.Subscription_State

// A topic resource.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type Topic = src.Topic

// UnimplementedPublisherServer can be embedded to have forward compatible
// implementations.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type UnimplementedPublisherServer = src.UnimplementedPublisherServer

// UnimplementedSchemaServiceServer can be embedded to have forward compatible
// implementations.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type UnimplementedSchemaServiceServer = src.UnimplementedSchemaServiceServer

// UnimplementedSubscriberServer can be embedded to have forward compatible
// implementations.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type UnimplementedSubscriberServer = src.UnimplementedSubscriberServer

// Request for the UpdateSnapshot method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type UpdateSnapshotRequest = src.UpdateSnapshotRequest

// Request for the UpdateSubscription method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type UpdateSubscriptionRequest = src.UpdateSubscriptionRequest

// Request for the UpdateTopic method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type UpdateTopicRequest = src.UpdateTopicRequest

// Request for the `ValidateMessage` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ValidateMessageRequest = src.ValidateMessageRequest
type ValidateMessageRequest_Name = src.ValidateMessageRequest_Name
type ValidateMessageRequest_Schema = src.ValidateMessageRequest_Schema

// Response for the `ValidateMessage` method. Empty for now.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ValidateMessageResponse = src.ValidateMessageResponse

// Request for the `ValidateSchema` method.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ValidateSchemaRequest = src.ValidateSchemaRequest

// Response for the `ValidateSchema` method. Empty for now.
//
// Deprecated: Please use types in: cloud.google.com/go/pubsub/apiv1/pubsubpb
type ValidateSchemaResponse = src.ValidateSchemaResponse

// Deprecated: Please use funcs in: cloud.google.com/go/pubsub/apiv1/pubsubpb
func NewPublisherClient(cc grpc.ClientConnInterface) PublisherClient {
	return src.NewPublisherClient(cc)
}

// Deprecated: Please use funcs in: cloud.google.com/go/pubsub/apiv1/pubsubpb
func NewSchemaServiceClient(cc grpc.ClientConnInterface) SchemaServiceClient {
	return src.NewSchemaServiceClient(cc)
}

// Deprecated: Please use funcs in: cloud.google.com/go/pubsub/apiv1/pubsubpb
func NewSubscriberClient(cc grpc.ClientConnInterface) SubscriberClient {
	return src.NewSubscriberClient(cc)
}

// Deprecated: Please use funcs in: cloud.google.com/go/pubsub/apiv1/pubsubpb
func RegisterPublisherServer(s *grpc.Server, srv PublisherServer) {
	src.RegisterPublisherServer(s, srv)
}

// Deprecated: Please use funcs in: cloud.google.com/go/pubsub/apiv1/pubsubpb
func RegisterSchemaServiceServer(s *grpc.Server, srv SchemaServiceServer) {
	src.RegisterSchemaServiceServer(s, srv)
}

// Deprecated: Please use funcs in: cloud.google.com/go/pubsub/apiv1/pubsubpb
func RegisterSubscriberServer(s *grpc.Server, srv SubscriberServer) {
	src.RegisterSubscriberServer(s, srv)
}