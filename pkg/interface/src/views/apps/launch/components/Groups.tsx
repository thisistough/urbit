import React from "react";
import { Box, Text } from "@tlon/indigo-react";

import { Associations, Association } from "~/types";
import { alphabeticalOrder } from "~/logic/lib/util";
import Tile from '../components/tiles/tile';

interface GroupsProps {
  associations: Associations;
}

const sortGroupsAlph = (a: Association, b: Association) =>
  alphabeticalOrder(a.metadata.title, b.metadata.title);

export default function Groups(props: GroupsProps & Parameters<typeof Box>[0]) {
  const { associations, invites, api, ...boxProps } = props;

  const incomingGroups = Object.values(invites?.['contacts'] || {});
  const getKeyByValue = (object, value) => {
    return Object.keys(object).find(key => object[key] === value);
  }

  const groups = Object.values(associations?.contacts || {})
    .filter(e => e['group-path'] in props.groups)
    .sort(sortGroupsAlph);

  const acceptInvite = (invite) => {
    const resource = {
      ship: `~${invite.resource.ship}`,
      name: invite.resource.name
    };
    return api.contacts.join(resource).then(() => {
      api.invite.accept('contacts', getKeyByValue(invites['contacts'], invite));
    });
  };

  return (
    <>
      {incomingGroups.map((invite) => (
        <Box
          height='100%'
          width='100%'
          bg='white'
          border='1'
          borderRadius='2'
          borderColor='lightGray'
          p='2'
          fontSize='0'>
          <Text display='block' pb='2' gray>You have been invited to:</Text>
          <Text
            display='inline-block'
            overflow='hidden'
            maxWidth='100%'
            style={{ textOverflow: 'ellipsis', whiteSpace: 'pre' }}
            title={`~${invite.resource.ship}/${invite.resource.name}`}>
              {`~${invite.resource.ship}/${invite.resource.name}`}
          </Text>
          <Box pt='5'>
            <Text
              onClick={() => acceptInvite(invite)}
            color='blue'
            mr='2'
            cursor='pointer'>
              Accept
            </Text>
            <Text
              color='red'
              onClick={() =>
                api.invite.decline(
                  'contacts',
                  getKeyByValue(invites['contacts'], invite)
                )
              }
              cursor='pointer'>
                Reject
              </Text>
          </Box>
        </Box>
      ))}
      {groups.map((group) => (
        <Tile to={`/~landscape${group["group-path"]}`}>
          <Text>{group.metadata.title}</Text>
        </Tile>
      ))}
    </>
  );
}
