from routing_base import RoutingBase
import sys

class WaterFillingRouting(RoutingBase):
	def __init__(self, sub_channel = ""):
		super().__init__(sub_channel)
		
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
		traffic_matrix = self.get_inter_region_traffic()
		decisions = []
		
		# load nodeid
		self.region_to_nodeIdStr = self.redis.hgetall('Region2NodeID')
		self.region_to_nodeId = {}
		for region in self.region_to_nodeIdStr:
			self.region_to_nodeId[str(region, 'utf-8')] = int(self.region_to_nodeIdStr[region])
		self.region_to_nodeIdStr = {}
		for region in self.region_to_nodeId:
			self.region_to_nodeIdStr[region] = str(self.region_to_nodeId[region])
	
		decisions.append(self.reroute_one_flow(traffic_matrix))
		decisions.append(self.reroute_one_flow(traffic_matrix))
		return decisions
		
	
	def reroute_one_flow(self, traffic_matrix, to_region = 'ap-southeast-1'):
		decision = {}
		flow_list = self.get_sorted_flow_by_time(traffic_matrix, to_region)
		from_region = flow_list[0][0]
		reroute_region = flow_list[-1][0]
		
		terminated = False
		for fetch_index in range(len(self.shuffle_details)):
			fetch = self.shuffle_details[fetch_index]
			fetcher = fetch['fetcher']
			if fetcher.split('.')[1] == to_region:
				for flow_info in fetch['flows']:
					sender = flow_info['src']
					size = flow_info['size']
					if sender.split('.')[1] == from_region:
						decision['SessionID'] = fetcher + "@" + sender
						if self.is_testing:
							decision['NodeID'] = from_region
							decision['NextHop'] = reroute_region
						else:
							decision['NodeID'] = self.region_to_nodeId[from_region]
							decision['NextHop'] = self.region_to_nodeId[reroute_region]
						# clean up
						del self.shuffle_details[fetch_index]
						traffic_matrix[to_region][from_region] -= size
						traffic_matrix[to_region][reroute_region] += size
						traffic_matrix[reroute_region][from_region] += size
						terminated = True
						break
			else:
				if terminated:
					break
	
		return decision
	

if __name__ == "__main__":
	if len(sys.argv) > 1:
		test = WaterFillingRouting(sys.argv[1])
	else:
		test = WaterFillingRouting()
		
	test.load_shuffle()
	test.send_decision()
