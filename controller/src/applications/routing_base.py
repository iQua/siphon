import sys
import json
import redis
import time

class RoutingBase():
	def __init__(self, sub_channel = ""):
		if (len(sub_channel) == 0):
			self.is_testing = True
		else:
			try:
				self.redis = redis.StrictRedis()
				self.sub = self.redis.pubsub()
				self.sub.subscribe(sub_channel)
				self.is_testing = False
			except Exception as e:
				print("Connection to Redis Error: " + str(e))
				print("Falling back to testing mode...")
				self.is_testing = True
	
	def load_shuffle(self):
		if (self.is_testing):
			# For testing purpose
			self.shuffle_details = json.load(open('./test_input.json'))[0]
			self.bandwidth_matrix = json.load(open('./test_input.json'))[1]
		else:
			self.region_to_nodeIdStr = self.redis.hgetall('Region2NodeID')
			self.region_to_nodeId = {}
			for region in self.region_to_nodeIdStr:
				self.region_to_nodeId[str(region, 'utf-8')] = int(self.region_to_nodeIdStr[region])
			self.region_to_nodeIdStr = {}
			for region in self.region_to_nodeId:
				self.region_to_nodeIdStr[region] = str(self.region_to_nodeId[region])
			
			self.bandwidth_matrix = json.load(open('./test_input.json'))[1]
			
			shuffle_details = ""
			while True:
				time.sleep(0.001)
				message = self.sub.get_message()
				if message is not None and message['type'] == 'message':
					shuffle_details = message['data']
					break
			self.shuffle_details = json.loads(str(shuffle_details, 'utf-8'))
			
	def _init_bandwidth_matrix(self):
		self.bandwidth_matrix = {}
		region_to_nodeId = self.region_to_nodeIdStr
		for to_region in region_to_nodeId:
			self.bandwidth_matrix[to_region] = {}
			bandwidth_to_region = self.redis.hgetall('BandwidthTo' + region_to_nodeId[to_region])
			for from_region in region_to_nodeId:
				if (from_region == to_region):
					continue
				self.bandwidth_matrix[to_region][from_region] = \
					float(str(bandwidth_to_region[region_to_nodeId[from_region].encode()], 'utf-8'))
					
	def get_inter_region_traffic(self, shuffle_details = None):
		if not shuffle_details:
			shuffle_details = self.shuffle_details
			
		traffic_matrix = {}
		for fetch_info in shuffle_details:
			to_region = fetch_info['fetcher'].split('.')[1]
			if not to_region in traffic_matrix:
				traffic_matrix[to_region] = {}
			for flow_info in fetch_info['flows']:
				from_region = flow_info['src'].split('.')[1]
				if (from_region != to_region):
					if not from_region in traffic_matrix[to_region]:
						traffic_matrix[to_region][from_region] = flow_info['size']
					else:
						traffic_matrix[to_region][from_region] += flow_info['size']
		
		return traffic_matrix
	
	def get_sorted_flow_by_time(self, traffic_matrix, fetch_region):
		"""
		:param traffic_matrix:
		:param fetch_region:
		:return: [(longest_flow_src, (size, bw)), (second_longest_flow_src, (size, bw)) ...]
		"""
		fetch_details = traffic_matrix[fetch_region]
		fetch_times = {}
		for src in fetch_details:
			fetch_times[src] = (fetch_details[src], self.bandwidth_matrix[fetch_region][src],
					fetch_details[src] / self.bandwidth_matrix[fetch_region][src] / 8.0 / 1024)
			
		return sorted(fetch_times.items(), key=lambda x: x[1][2], reverse=True)
	
	def _compute(self):
		"""
		Run the actual algorithm.
		Input
			* Bandwidth matrix -- self.bandwidth_matrix
			* Traffic matrix -- get_inter_region_traffic()
			* Region name to nodeID map -- self.region_to_nodeId
			
		return: A dictionary of messages, to be sent to each node
			Sample: `{1: [dst1, dst2, dst3]} # simple next hop`
			Sample: `{1:  [{"NextHop": dst0, "Weight": w0 }, {"NextHop": dst1, "Weight": w1 }...]} # split`
		"""
		return {1: [2]}
	
	def send_decision(self):
		decision_list = self._compute()
		for decision in decision_list:
			message_dict = {
				'Type': 5,
				'Msg': {
					'SessionID': decision['SessionID'],
					'Entry': [decision['NextHop']],
					'Timeout': 10
				}
			}
			node = decision['NodeID']
			message = json.dumps(message_dict)
			if self.is_testing:
				print(node, " receive ", message)
			else:
				self.redis.publish(str(node), message)
			
if __name__ == "__main__":
	if len(sys.argv) > 2:
		test = RoutingBase(sys.argv[1])
	else:
		test = RoutingBase()
		
	test.load_shuffle() # should blocking wait until a report
	print(test.get_inter_region_traffic())
	for region in test.bandwidth_matrix:
		print(region, " : ", test.get_sorted_flow_by_time(test.get_inter_region_traffic(), region))
	test.send_decision()